# Top-Level Makefile

all:
	west build

flash:
	west flash

clean:
	rm -rf build

# Docker Compose
docker-up:
	docker compose up -d

docker-down:
	docker compose down

docker-build:
	docker compose build

docker-rebuild:
	docker compose down && docker compose build && docker compose up -d

docker-shell:
	docker compose exec fsw-dev zsh

docker-logs:
	docker compose logs -f

docker-clean:
	docker compose down -v --rmi local

# Docker development shortcuts
dev-start: docker-up
	@echo "Development environment started. Use 'make docker-shell' to enter."

dev-stop: docker-down
	@echo "Development environment stopped."

dev-restart: docker-rebuild
	@echo "Development environment rebuilt and restarted."

include app/backplane/Makefile
include app/ground/Makefile
include app/payload/Makefile
include app/other/Makefile
