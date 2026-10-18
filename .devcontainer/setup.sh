#!/bin/bash
set -e

# Fix potential permissions issues for bind mounts (host-to-container UID mismatch)
echo "🔑 Fixing permissions for /workspace..."
sudo chown -R $(id -u):$(id -g) /workspace

echo "🚀 Setting up Launch FSW development environment..."

# Navigate to workspace
cd /workspace

# Check west availability
echo "📦 Checking west installation..."
west --version || (pipx ensurepath && west --version)

# Update west workspace
if [ -d .west ]; then
    echo "🔄 West workspace exists, running update..."
    west update || true
else
    echo "🆕 Initializing west workspace with FSW manifest..."
    if [ -f FSW/west.yml ]; then
        west init -l FSW
        west update || true
    else
        echo "❌ No west.yml found in FSW directory"
        exit 1
    fi
fi

# Set up Zephyr environment
if [ -d zephyr ]; then
    echo "🔧 Setting up ZEPHYR_BASE environment..."
    echo "export ZEPHYR_BASE=/workspace/zephyr" | sudo tee /etc/profile.d/zephyr_base.sh >/dev/null
fi

# Create Python virtual environment for Zephyr
echo "🐍 Setting up Python virtual environment..."
VENV_DIR="$HOME/.venvs/zephyr"
[ -d "$VENV_DIR" ] || python3 -m venv "$VENV_DIR"

# Activate venv and install requirements
. "$VENV_DIR/bin/activate"
pip install -U pip

if [ -f zephyr/scripts/requirements.txt ]; then
    echo "📋 Installing Zephyr Python requirements..."
    pip install -r zephyr/scripts/requirements.txt
fi

# Export Zephyr modules
echo "🔗 Running west zephyr-export..."
west zephyr-export || true

# Auto-activate venv in zsh
if ! grep -q "source $HOME/.venvs/zephyr/bin/activate" "$HOME/.zshrc" 2>/dev/null; then
    echo "Configuring zsh to auto-activate Python environment..."
    echo "source $HOME/.venvs/zephyr/bin/activate" >> "$HOME/.zshrc"
fi

echo "Development environment setup complete!"
west --version
