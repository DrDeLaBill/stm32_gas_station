#define MB_PORT_INDEX 2

#define MB_PORT_SPEED 115200

#define MB_BUF_SIZE   256

#define MB_PORT_STOP  0

main()
{
    PortInit(MB_PORT_INDEX, MB_PORT_SPEED, MB_BUF_SIZE, MB_PORT_STOP)
}