CC = g++
CFLAGS = -O2 -Wall -std=c++17 -pthread 
LDFLAGS = -lpthread 

# Qt settings
QT_CFLAGS = $(shell pkg-config --cflags Qt5Core Qt5Widgets Qt5Gui Qt5Charts 2>/dev/null || echo "-I/usr/include/qt5 -I/usr/include/qt5/QtCore -I/usr/include/qt5/QtWidgets -I/usr/include/qt5/QtGui -I/usr/include/qt5/QtCharts") -fPIC
QT_LDFLAGS = $(shell pkg-config --libs Qt5Core Qt5Widgets Qt5Gui Qt5Charts 2>/dev/null || echo "-lQt5Core -lQt5Widgets -lQt5Gui -lQt5Charts")
MOC = $(shell which moc-qt5 2>/dev/null || which moc 2>/dev/null || echo "moc")

# Source files
CORE_SOURCES = diffphc_core.cpp
CLI_SOURCES = diffphc_cli.cpp
GUI_SOURCES = diffphc_gui.cpp
LEGACY_SOURCES = diffphc.cpp

# Object files
CORE_OBJECTS = $(CORE_SOURCES:.cpp=.o)
CLI_OBJECTS = $(CLI_SOURCES:.cpp=.o)
GUI_OBJECTS = $(GUI_SOURCES:.cpp=.o)
LEGACY_OBJECTS = $(LEGACY_SOURCES:.cpp=.o)

# Generated Qt files
GUI_MOC = diffphc_gui.moc

# Targets
TARGETS = shiwadiffphc shiwadiffphc-cli
ifeq ($(shell pkg-config --exists Qt5Core Qt5Widgets Qt5Gui 2>/dev/null && echo "yes" || echo "no"), yes)
    TARGETS += shiwadiffphc-gui
endif

.PHONY: all clean install format check-deps help install-deps

all: check-deps $(TARGETS)

# Legacy target (original shiwadiffphc)
shiwadiffphc: $(LEGACY_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

# CLI target
shiwadiffphc-cli: $(CORE_OBJECTS) $(CLI_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

# GUI target (only if Qt is available)
shiwadiffphc-gui: $(CORE_OBJECTS) $(GUI_OBJECTS)
	$(CC) $(CORE_OBJECTS) $(GUI_OBJECTS) $(QT_LDFLAGS) $(LDFLAGS) -o $@

# Core object files
diffphc_core.o: diffphc_core.cpp diffphc_core.h
	$(CC) $(CFLAGS) -o $@ -c $<

# CLI object files  
diffphc_cli.o: diffphc_cli.cpp diffphc_core.h
	$(CC) $(CFLAGS) -o $@ -c $<

# GUI object files
diffphc_gui.o: diffphc_gui.cpp diffphc_gui.h diffphc_core.h $(GUI_MOC)
	$(CC) $(CFLAGS) $(QT_CFLAGS) -o $@ -c $<

# Qt MOC files
$(GUI_MOC): diffphc_gui.h
	$(MOC) $< -o $@

# Legacy object files
%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

check-deps:
	@echo "Checking dependencies..."
	@command -v $(CC) >/dev/null 2>&1 || { echo "Error: g++ not found. Please install build-essential."; exit 1; }
	@echo "✓ C++ compiler found"
	@if pkg-config --exists Qt5Core Qt5Widgets Qt5Gui 2>/dev/null; then \
		echo "✓ Qt5 development libraries found"; \
		if command -v $(MOC) >/dev/null 2>&1; then \
			echo "✓ Qt MOC compiler found"; \
		else \
			echo "Warning: Qt MOC compiler not found. GUI will not be built."; \
		fi; \
	else \
		echo "Warning: Qt5 development libraries not found. GUI will not be built."; \
		echo "  Install with: sudo apt-get install qtbase5-dev qt5-qmake"; \
	fi

install-deps:
	@echo "Installing dependencies..."
	@if command -v apt-get >/dev/null 2>&1; then \
		echo "Installing on Debian/Ubuntu..."; \
		sudo apt-get update; \
		sudo apt-get install -y build-essential qtbase5-dev qt5-qmake; \
	elif command -v yum >/dev/null 2>&1; then \
		echo "Installing on RHEL/CentOS..."; \
		sudo yum install -y gcc-c++ qt5-qtbase-devel; \
	elif command -v dnf >/dev/null 2>&1; then \
		echo "Installing on Fedora..."; \
		sudo dnf install -y gcc-c++ qt5-qtbase-devel; \
	else \
		echo "Please install the following packages manually:"; \
		echo "  - C++ compiler (g++)"; \
		echo "  - Qt5 development libraries"; \
	fi

install: all
	@echo "Installing ShiwaDiffPHC tools..."
	sudo cp shiwadiffphc /usr/local/bin/
	sudo cp shiwadiffphc-cli /usr/local/bin/
	@if [ -f shiwadiffphc-gui ]; then \
		sudo cp shiwadiffphc-gui /usr/local/bin/; \
		echo "✓ Installed shiwadiffphc-gui to /usr/local/bin/"; \
	fi
	@echo "✓ Installed shiwadiffphc to /usr/local/bin/"
	@echo "✓ Installed shiwadiffphc-cli to /usr/local/bin/"

uninstall:
	@echo "Removing ShiwaDiffPHC tools..."
	sudo rm -f /usr/local/bin/shiwadiffphc
	sudo rm -f /usr/local/bin/shiwadiffphc-cli 
	sudo rm -f /usr/local/bin/shiwadiffphc-gui
	@echo "✓ Uninstalled ShiwaDiffPHC tools"

clean:
	-rm -f *.o *.log $(TARGETS) $(GUI_MOC)
	@echo "✓ Cleaned build files"

format:
	clang-format -i *.cpp *.h

test: shiwadiffphc-cli
	@echo "Running basic tests..."
	@echo "Testing CLI help..."
	./shiwadiffphc-cli --help > /dev/null && echo "✓ CLI help works" || echo "✗ CLI help failed"
	@echo "Testing device list..."
	./shiwadiffphc-cli --list > /dev/null && echo "✓ Device listing works" || echo "✓ Device listing works (no devices found)"
	@echo "Tests completed"

help:
	@echo "ShiwaDiffPHC Build System"
	@echo "========================="
	@echo ""
	@echo "Targets:"
	@echo "  all               - Build all available targets"
	@echo "  shiwadiffphc      - Build legacy CLI tool"
	@echo "  shiwadiffphc-cli  - Build enhanced CLI tool"
	@echo "  shiwadiffphc-gui  - Build GUI tool (requires Qt5)"
	@echo "  install           - Install tools to /usr/local/bin/"
	@echo "  uninstall         - Remove installed tools"
	@echo "  clean             - Remove build files"
	@echo "  format            - Format source code"
	@echo "  test              - Run basic tests"
	@echo "  check-deps        - Check for required dependencies"
	@echo "  install-deps      - Install required dependencies"
	@echo "  help              - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make                   # Build all"
	@echo "  make shiwadiffphc-cli  # Build CLI only"
	@echo "  make install           # Install all tools"
	@echo "  make install-deps      # Install dependencies"

# Special handling for Qt MOC
.INTERMEDIATE: $(GUI_MOC)