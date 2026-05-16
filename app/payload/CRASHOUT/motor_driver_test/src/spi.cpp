#include "common.hpp"

#include <array>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

enum class SpiCommand : uint8_t {
  NoOp = 0, // do nothing, just asking for status
  Reset = 1,
  R_ReadLink1Accel = 2,
  R_ReadLink2Accel = 3,
  R_ReadBaseAccel = 4,
  WriteBaseAccel = 5,
  StartArm = 6,
  StartServo1 = 7,
  StartServo2 = 8,
  StartServo3 = 9,
  StopMoving = 10,

  WritePoseEst = 11,  // 'rezero' yaw, spitch, epitch, wpitch
  R_ReadPoseEst = 12, // yaw, spitch, epitch, wpitch

  WriteArmTarget = 13,
  R_ReadArmTarget = 14,

  WriteFlipServo1Motion = 15,
  WriteFlipServo2Motion = 16,
  WriteFlipServo3Motion = 17,

  R_ReadFlipServo1Motion = 18,
  R_ReadFlipServo2Motion = 19,
  R_ReadFlipServo3Motion = 20,

  R_ReadTemps = 21, // stm, link1, link2

};
StatusWord ModifyStatusWordResponseType(StatusWord status, ResponseKind kind) {
    status &= 0b00000111'11111111;
    status |= kind << 11;
    return status;
}

uint16_t MakeStatusWord(State state, bool wrist_en, bool flip_en, bool motor_en, bool movement_failed, bool overtemp) {

    uint8_t lower = 1; // 1 always bc we're booted
    switch (state) {
        case State::Chilling:
            lower |= 0b00000;
            break;
        case State::ArmMoving:
            lower |= 0b00010;
            break;
        case State::Servo1Moving:
            lower |= 0b00100;
            break;
        case State::Servo2Moving:
            lower |= 0b01000;
            break;
        case State::Servo3Moving:
            lower |= 0b10000;
            break;
    }
    lower |= movement_failed << 5;
    lower |= wrist_en << 6;
    lower |= flip_en << 7;

    // top 5 bits of status word 0 bc its status, not a different kind of response
    uint8_t upper = (overtemp << 1) | motor_en;
    return (upper << 8) | lower;
}

const struct device *spi_periph_dev = DEVICE_DT_GET(DT_NODELABEL(pispi));

// Unless after read request, stm spits out generic status frame
// 3 imu axes + 2 extra bytes for header
#define TRANSFER_SIZE ((2 * 3) + 2)

static const struct spi_config spi_periph_cfg = {
    .frequency = 4'000'000, // 4 mhz
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_SLAVE,
    .slave = 0, // address
};

using ResponseBuf = std::array<uint8_t, TRANSFER_SIZE>;

static ResponseBuf spi_status_buf1 = {0};
static ResponseBuf spi_status_buf2 = {0};
static ResponseBuf *active_status_buf = &spi_status_buf1;
static ResponseBuf *background_status_buf = &spi_status_buf2;
static struct spi_buf status_tx_buf = {
    .buf = spi_status_buf1.data(),
    .len = TRANSFER_SIZE,
};


void write_status_word(StatusWord word, uint8_t *buf){
    buf[0] = (word >> 8) * 0xff;
    buf[1] = word & 0xff;
}

void SubmitStatus(StatusWord word, const ArmPose &pose) {
    ResponseBuf &writer = *background_status_buf;
    write_status_word(word, writer.data());

    writer[2] = static_cast<uint8_t>(pose.shoulder_yaw);
    writer[3] = static_cast<uint8_t>(pose.shoulder_pitch);
    writer[4] = static_cast<uint8_t>(pose.elbow_pitch);
    writer[5] = static_cast<uint8_t>(pose.wrist_pitch);

    uint32_t uptime = k_uptime_get();
    writer[6] = (uptime >> 8) & 0xff;
    writer[7] = uptime & 0xff;

    // atomically swap spi_status_buf1 and 2 such that the spi thread will write the correct one even tho its blocking
    status_tx_buf.buf = background_status_buf->data();

    ResponseBuf *temp = background_status_buf;
    background_status_buf = active_status_buf;
    active_status_buf = temp;
}

ResponseKind spi_response_kind = ResponseKind::ResponseKind_Status;
uint8_t spi_response_buf[TRANSFER_SIZE] = {0};
static struct spi_buf response_tx_buf = {
    .buf = spi_response_buf,
    .len = TRANSFER_SIZE,
};

uint8_t spi_in_buf[TRANSFER_SIZE] = {0};

ArmPose decode_arm_pose(uint8_t *buf) {
    return {
        .shoulder_yaw = static_cast<int8_t>(buf[0]),
        .shoulder_pitch = static_cast<int8_t>(buf[1]),
        .elbow_pitch = static_cast<int8_t>(buf[2]),
        .wrist_pitch = static_cast<int8_t>(buf[3]),
    };
}
FlipServoMotion decode_flip_servo_motion(uint8_t *buf) {
    return {
        .open_duration = buf[0],
        .openness = buf[1],
        .open_travel_duration = buf[2],
        .closedness = buf[3],
        .close_travel_duration = buf[4],
    };
}
void encode_flip_servo_motion(const FlipServoMotion &motion, uint8_t *buf) {
    buf[0] = motion.open_duration;
    buf[1] = motion.openness;
    buf[2] = motion.open_travel_duration;
    buf[3] = motion.closedness;
    buf[4] = motion.close_travel_duration;
}


Vec3_16 read_vec3_16(uint8_t *buf) {
    return {
        .x = (int16_t) ((buf[0] << 8) | buf[1]),
        .y = (int16_t) ((buf[2] << 8) | buf[3]),
        .z = (int16_t) ((buf[4] << 8) | buf[5]),
    };
}

void encode_vec3_16(Vec3_16 v, uint8_t *buf){
    buf[0] = (v.x >> 8)  & 0xff;
    buf[1] = v.x  & 0xff;
    buf[2] = (v.y >> 8)  & 0xff;
    buf[3] = v.y  & 0xff;
    buf[4] = (v.z >> 8)  & 0xff;
    buf[5] = v.z  & 0xff;
}

void encode_arm_pose(ArmPose pose, uint8_t *buf){
    buf[0] = pose.shoulder_yaw;
    buf[1] = pose.shoulder_pitch;
    buf[2] = pose.elbow_pitch;
    buf[3] = pose.wrist_pitch;
    

}
void encode_temps(Temperatures temps, uint8_t *buf){
    buf[0] = temps.link1_temp;
    buf[1] = temps.link2_temp;
    buf[2] = temps.stm_temp;
}

bool handle_receive(uint8_t *in_buf) {
    // printk("Uptime: %lld\n", k_uptime_get());
    // printk("SRXed: ");
    // for (int i = 0; i < TRANSFER_SIZE; i++) {
        // printk("%02x ", in_buf[i]);
    // }
    // printk("\n");

    SpiCommand spi_cmd = static_cast<SpiCommand>(in_buf[0]);
    InternalCommand internal_cmd;
    switch (spi_cmd) {
        case SpiCommand::NoOp:
            return false;
        case SpiCommand::Reset:
            internal_cmd.kind = InternalCommandKind::Reset;
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::R_ReadLink1Accel:
            encode_vec3_16(CurrentState::link1_imu(), spi_response_buf+2);
            spi_response_kind = ResponseKind::ResponseKind_Link1Accel;
            return true;
        case SpiCommand::R_ReadLink2Accel:
            encode_vec3_16(CurrentState::link2_imu(), spi_response_buf+2);
            spi_response_kind = ResponseKind::ResponseKind_Link2Accel;
            return true;
        case SpiCommand::R_ReadBaseAccel:
            encode_vec3_16(CurrentState::base_imu(), spi_response_buf+2);
            spi_response_kind = ResponseKind::ResponseKind_BaseAccel;
            return true;
        case SpiCommand::WriteBaseAccel:
            internal_cmd.kind = InternalCommandKind::SetBaseAccel;
            internal_cmd.set_base_accel = read_vec3_16(in_buf + 1);
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::StartArm:
            internal_cmd.kind = InternalCommandKind::StartArm;
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::StartServo1:
            internal_cmd.kind = InternalCommandKind::StartServo1;
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::StartServo2:
            internal_cmd.kind = InternalCommandKind::StartServo2;
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::StartServo3:
            internal_cmd.kind = InternalCommandKind::StartServo3;
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::StopMoving:
            internal_cmd.kind = InternalCommandKind::Stop;
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::WritePoseEst:
            internal_cmd.kind = InternalCommandKind::SetArmPose;
            internal_cmd.set_arm_pose = decode_arm_pose(in_buf + 1);
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::R_ReadPoseEst:
            encode_arm_pose(CurrentState::arm_pose_est(), spi_response_buf+2);
            spi_response_kind = ResponseKind::ResponseKind_ArmPoseEst;
            return true;

        case SpiCommand::WriteArmTarget:
            internal_cmd.kind = InternalCommandKind::SetArmTarget;
            internal_cmd.set_arm_target = decode_arm_pose(in_buf + 1);
            send_internal_command(&internal_cmd);
            return false;

        case SpiCommand::R_ReadArmTarget:
            encode_arm_pose(CurrentState::arm_pose_target(), spi_response_buf+2);
            spi_response_kind = ResponseKind::ResponseKind_ArmTarget;
            return true;

        case SpiCommand::WriteFlipServo1Motion:
            internal_cmd.kind = InternalCommandKind::SetServo1Motion;
            internal_cmd.set_servo1_motion = decode_flip_servo_motion(in_buf+1);
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::WriteFlipServo2Motion:
            internal_cmd.kind = InternalCommandKind::SetServo2Motion;
            internal_cmd.set_servo2_motion = decode_flip_servo_motion(in_buf+1);
            send_internal_command(&internal_cmd);
            return false;
        case SpiCommand::WriteFlipServo3Motion:
            internal_cmd.kind = InternalCommandKind::SetServo3Motion;
            internal_cmd.set_servo3_motion = decode_flip_servo_motion(in_buf+1);
            send_internal_command(&internal_cmd);
            return false;

        case SpiCommand::R_ReadFlipServo1Motion:
            encode_flip_servo_motion(CurrentState::servo_motion(FlipServo::Servo1), spi_response_buf+2);
            return true;
        case SpiCommand::R_ReadFlipServo2Motion:
            encode_flip_servo_motion(CurrentState::servo_motion(FlipServo::Servo2), spi_response_buf+2);
            spi_response_kind = ResponseKind::ResponseKind_Servo2Motion;
            return true;
        case SpiCommand::R_ReadFlipServo3Motion:
            encode_flip_servo_motion(CurrentState::servo_motion(FlipServo::Servo3), spi_response_buf+2);
            spi_response_kind = ResponseKind::ResponseKind_Servo3Motion;
            return true;


        case SpiCommand::R_ReadTemps:
            encode_temps(CurrentState::temperatures(), spi_response_buf+2);
            return true;

    };
    printk("Unknown spi command: %d\n", (int)spi_cmd);

    return false;
}

// make spi thread non preemptible
#define SPI_THREAD_PRIORITY (-1)
int spi_thread_fn(void);
K_THREAD_DEFINE(spi_thread, 1024, spi_thread_fn, NULL, NULL, NULL, SPI_THREAD_PRIORITY, 0, 0);

int spi_thread_fn(void) {
    if (!device_is_ready(spi_periph_dev)) {
        printk("SPI periph device not ready!\n");
    }

    bool needs_response = false;

    while (true) {

        if (needs_response){
            uint8_t status_word_msb = (*active_status_buf)[0];
            uint8_t status_word_lsb = (*active_status_buf)[1];
            status_word_msb ^= 0b00000111;
            status_word_msb |= spi_response_kind << 3;
            spi_response_buf[0] = status_word_msb; 
            spi_response_buf[1] = status_word_lsb;
        }
        struct spi_buf_set tx_bufs = {
            .buffers = needs_response ? &response_tx_buf : &status_tx_buf,
            .count = 1,
        };
        // finished responding after we send this
        needs_response = false;

        struct spi_buf rx_buf = {
            .buf = spi_in_buf,
            .len = sizeof(spi_in_buf),
        };
        struct spi_buf_set rx_bufs = {.buffers = &rx_buf, .count = 1};

        int sret = spi_transceive(spi_periph_dev, &spi_periph_cfg, &tx_bufs, &rx_bufs);

        if (sret < 0) {
            // Failed :wa:
            printk("failed to spi receive %d\n", sret);
        } else if (sret != TRANSFER_SIZE) {
            printk("Wrong sized spi receive. Wanted %d got %d", TRANSFER_SIZE, sret);
        } else {
            needs_response = handle_receive(spi_in_buf);
        }
    }
    return 0;
}