.PHONY: build install clean delete

TARGET = disk_daemon
SRC = main.c

OPENRC_SERVICE = service/disk_daemon
SYSTEMD_SERVICE = service/disk_daemon.service

OPENRC_DEST = /etc/init.d/disk_daemon
SYSTEMD_DEST = /etc/systemd/system/disk_daemon.service

BIN_DEST = /usr/bin/$(TARGET)


build:
	gcc -o $(TARGET) $(SRC)

install:
	@init=$$(ps -p 1 -o comm=); \
	if [ "$$init" = "systemd" ]; then \
		echo systemd; \
		sudo cp $(SYSTEMD_SERVICE) $(SYSTEMD_DEST); \
	elif [ "$$init" = "init" ]; then \
		echo openrc; \
		sudo cp $(OPENRC_SERVICE) $(OPENRC_DEST); \
		sudo chmod +x $(OPENRC_DEST); \
	else \
		echo unknown init; \
		exit 1; \
	fi; \
	sudo cp $(TARGET) $(BIN_DEST)

delete:
	@init=$$(ps -p 1 -o comm=); \
	if [ "$$init" = "systemd" ]; then \
		echo "systemd service & bin delete"; \
		sudo rm -f $(SYSTEMD_DEST); \
	elif [ "$$init" = "init" ]; then \
		echo "openrc script & bin delete"; \
		sudo rm -f $(OPENRC_DEST); \
	else \
		echo unknown init; \
		exit 1; \
	fi; \
	sudo rm -f $(BIN_DEST)

clean:
	rm -f $(TARGET)
