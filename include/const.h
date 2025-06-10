// Direccion MAC del receptor
const uint8_t MAC_RECEIVER[] = { 0x0C, 0xB8, 0x15, 0xD7, 0xE5, 0x38 };

// Asignamos un valor para cada tipo de dispositivo
enum DeviceType {
    CONTROL_TRANSMITTER = 1,
    RECEIVING_DEVICE = 2,
    TRUNK_PROTECTOR_TRANSMITTER = 3
};

// Lista para la configuracion del dispositivo receptor
enum SYSTEM_MODE {
    ONE_CONTROL = 0,
    TWO_CONTROL = 1,
    JUST_TRUNK_PROTECTOR = 2,
    TRUNK_PROTECTOR_OR_CONTROL = 3,
    TRUNK_PROTECTOR_WITH_CONTROL_CONFIRMATION = 4
};

// Lista de los estados posibles de la botonera
enum CONTROL_STATE {
    NO_ACTION = 0,
    BLUE_PUNCH = 1,
    BLUE_BODY_KICK = 2,
    BLUE_HEAD_KICK = 3,
    BLUE_BODY_TECHNICAL_KICK = 4,
    BLUE_HEAD_TECHNICAL_KICK = 5,
    RED_PUNCH = 6,
    RED_BODY_KICK = 7,
    RED_HEAD_KICK = 8,
    RED_BODY_TECHNICAL_KICK = 9,
    RED_HEAD_TECHNICAL_KICK = 10,
};

enum PLAYER_COLOR {
    PLAYER_BLUE = false,
    PLAYER_RED = true
};