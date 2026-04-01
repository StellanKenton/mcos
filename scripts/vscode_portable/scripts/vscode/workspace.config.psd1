@{
    ProjectPath = 'Project/MDK-ARM/mcos-gd32.uvprojx'
    KeilTarget = 'MCOS-GD32'
    HexFilePath = 'Project/MDK-ARM/GD32F4xx-OBJ/mcos-gd32.hex'

    JLink = @{
        Device = 'GD32F407VG'
        Interface = 'SWD'
        SpeedKHz = 10000
        GdbPort = 2331
        SwoPort = 2332
        TelnetPort = 2333
        RttTelnetPort = 19021
    }
}