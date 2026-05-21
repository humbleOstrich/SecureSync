set -e

# Установка системных зависимостей (компилятор, make)
install_build_deps() {
    if command -v apt-get >/dev/null 2>&1; then
        apt-get update
        apt-get install -y make g++
    elif command -v dnf >/dev/null 2>&1; then
        dnf install -y make gcc-c++
    elif command -v yum >/dev/null 2>&1; then
        yum install -y make gcc-c++
    elif command -v pacman >/dev/null 2>&1; then
        pacman -S --noconfirm make gcc
    elif command -v apk >/dev/null 2>&1; then
        apk add make g++
    elif command -v zypper >/dev/null 2>&1; then
        zypper install -y make gcc-c++
    else
        echo "Warning: unsupported package manager, please install make and g++ manually." >&2
    fi
}

# Установка Qt (если qmake не найден)
install_qt() {
    if command -v qmake6 >/dev/null 2>&1 || command -v qmake >/dev/null 2>&1; then
        echo "Qt found, skipping Qt installation."
        return 0
    fi
    echo "Qt not found. Attempting to install..."
    if command -v apt-get >/dev/null 2>&1; then
        apt-get update
        apt-get install -y qt6-base-dev qt6-tools-dev || apt-get install -y qt5-qmake qtbase5-dev
    elif command -v dnf >/dev/null 2>&1; then
        dnf install -y qt6-qtbase-devel || dnf install -y qt5-qtbase-devel
    elif command -v yum >/dev/null 2>&1; then
        yum install -y qt6-qtbase-devel || yum install -y qt5-qtbase-devel
    elif command -v pacman >/dev/null 2>&1; then
        pacman -S --noconfirm qt6-base || pacman -S --noconfirm qt5-base
    elif command -v apk >/dev/null 2>&1; then
        apk add qt6-qtbase-dev || apk add qt5-qtbase-dev
    else
        echo "Error: cannot install Qt automatically. Please install Qt5 or Qt6 manually." >&2
        exit 1
    fi
}

# Проверка и установка базовых инструментов
if ! command -v make >/dev/null 2>&1 || ! command -v g++ >/dev/null 2>&1; then
    install_build_deps
fi

# Проверка и установка Qt для сборки GUI клиента
install_qt

# Запуск make для сборки библиотеки, сервера, консольного клиента и GUI клиента
make
