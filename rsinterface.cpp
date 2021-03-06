//This is an open source non-commercial project. Dear PVS-Studio, please check it.

#include "rsinterface.h"
#include <thread>


RSInterface::RSInterface(TypeParams _params, PtrSerial _serial):
    IInterface(_params),    
    _channelId(-1),
    serial(_serial)
{
    params = std::dynamic_pointer_cast<ParamsRS232>(_params);

    if(params)
    {
        //std::cout << "constructor RSInterface devPath=" << params->getDevPath() << std::endl;
    }
    else
    {
        std::cout << "Fatal Error get TypeParamsRS. Exit program" << std::endl;
        exit(0);
    }
}

RSInterface::~RSInterface()
{
}

bool RSInterface::open()
{
    std::cout << "RSInterface::open() " << params->getDevPath() << std::endl;
    //std::cout << "params->get9thBit() " << params->get9thBit() << std::endl;
    isFirstByte = true;

    _channelId = serial->open(params->getDevPath().c_str(), /*O_RDWR*/ O_RDWR | O_NOCTTY | O_NDELAY);

    if (_channelId < 0)
    {
        std::cout << "Can't open device" << params->getDevPath() << std::endl;
        perror(params->getDevPath().c_str());
        return false;
    }

    memset (&newtio0, 0, sizeof (newtio0));


    if(params->get9thBit())
    {
        // 9th bit
        newtio0.c_iflag = IGNBRK;

        newtio0.c_iflag &= ~IGNPAR;
        newtio0.c_iflag &= ~ISTRIP;
        newtio0.c_iflag |= INPCK;

        newtio0.c_iflag |= PARMRK; // Mark all bytes received with 9th bit set by "ff 0"
        //newtio0.c_cflag |= CMSPAR;

        newtio0.c_cflag &= ~PARODD;	// normal state - space parity

        //newtio0.c_iflag &= ~(IXON | IXOFF | IXANY);
        newtio0.c_cflag = CREAD | CLOCAL;
        newtio0.c_cflag |= PARENB;// | PARODD;
        newtio0.c_cflag &= ~CSTOPB;
        newtio0.c_cflag &= ~CSIZE;
        newtio0.c_cflag |= CS8;
        newtio0.c_cflag &= ~CRTSCTS;
        newtio0.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        newtio0.c_oflag &= ~OPOST;
        newtio0.c_cc[VMIN] = 0;
        newtio0.c_cc[VTIME]= 0;

        serial->cfsetospeed( params->getBaudRate() );
        serial->cfsetispeed( params->getBaudRate() );

        //todo tcflush (_channelId, TCIFLUSH);
        //tcsetattr(_channelId, TCSANOW, &newtio0);
        serial->tcsetattr (_channelId, &newtio0);


        // Set receive with space parity
        newtio0.c_cflag |= PARENB;
        //newtio0.c_cflag |= CMSPAR;
        newtio0.c_cflag &= ~PARODD;

        //tcsetattr(_channelId, TCSANOW, &newtio0);
        serial->tcsetattr(_channelId, &newtio0);
    }
    else
    {
        serial->cfsetospeed( params->getBaudRate() );
        serial->cfsetispeed( params->getBaudRate() );

        newtio0.c_cflag = params->getByteSize() | CLOCAL | CREAD | (int)params->getParity();
        newtio0.c_oflag = 0;
        newtio0.c_lflag = 0;
        newtio0.c_cc[VTIME] = 10;    // inter-character timer unused
        newtio0.c_cc[VMIN]  = 0;    // blocking read until  chars received


        newtio0.c_cflag &= ~CRTSCTS;                                                   // disable hardware flow control
        newtio0.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);                            // raw mode!
        newtio0.c_iflag &= ~(INPCK | ISTRIP | IUCLC | IGNCR | ICRNL | INLCR | PARMRK); // raw mode!
        newtio0.c_iflag &= ~(IXON | IXOFF | IXANY);                                    // disable software flow control
        newtio0.c_oflag &= ~OPOST;

        serial->tcsetattr (_channelId, &newtio0);
    }


//            std::cout <<"Set Pasha settings" << std::endl;
//            //_channelId = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);


//            fcntl(_channelId, F_SETFL, FNDELAY); // read with no delay
//            tcgetattr(_channelId, &newtio0);

//            cfsetispeed(&newtio0, params->getBaudRate());
//            cfsetospeed(&newtio0, params->getBaudRate());

//            newtio0.c_cflag |= (CLOCAL | CREAD);
//            newtio0.c_cflag |= PARENB;
//            //(int)params->getParity();//PARENB; // enable parity bit
//            newtio0.c_cflag &= ~PARODD;

//            newtio0.c_cflag &= ~CSTOPB;
//            newtio0.c_cflag &= ~CSIZE;                                                     // bit mask for data bits
//            newtio0.c_cflag |= params->getByteSize();                                                        // 8 data bits
//            newtio0.c_cflag &= ~CRTSCTS;                                                   // disable hardware flow control
//            newtio0.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);                            // raw mode!
//            newtio0.c_iflag &= ~(INPCK | ISTRIP | IUCLC | IGNCR | ICRNL | INLCR | PARMRK); // raw mode!
//            newtio0.c_iflag &= ~(IXON | IXOFF | IXANY);                                    // disable software flow control
//            newtio0.c_oflag &= ~OPOST;
//            newtio0.c_cflag &= ~CBAUD;
//            newtio0.c_cflag |= B9600;
//            tcsetattr(_channelId, TCSANOW, &newtio0);

//            /*********************************/
//            sleep(2); // required to make flush work, for some reason
//            tcflush(_channelId, TCIOFLUSH);


    return true;
}

bool RSInterface::close()
{
    return (serial->close() == 0)?true:false;
}

int RSInterface::read(char *data, int size, int timeout)
{
    if(serial->select(static_cast<size_t>(timeout)) > 0)
    {
        return static_cast<int>(serial->read(data, static_cast<size_t>(size)));
    }
    return 0;
}

int RSInterface::write(const char *data, int size)
{
    int res = 0;
    int offset = 0;

    if(!data)
    {
        return -1;
    }

//    if(params->get9thBit() && isFirstByte)
//    {
//        isFirstByte = false;

//        if((res = putCharWakeup(data[0])) == -1)
//        {
//            return -1;
//        }

//        offset = 1;
//        size--;

//    }

    res += serial->write(data + offset, size);

    //std::cout << "write res=" << res << std::endl;

    return res;
}

/******************************************************************************
** SRL_PutCharWakeup
**
** Description:	put char with wakeup bit
** Input:		symbol - characted to be sent
** Return:		 1 - OK
**               0 - nothing wrote
**				-1 - ERROR
******************************************************************************/
int RSInterface::putCharWakeup(unsigned char /*symbol*/)
{            
    int				rc=0;

//    unsigned char	tmp, nine;

//    tmp  = symbol;
//    nine = 0;

//    std::cout << "SRL_PutCharWakeup symbol = " << (int)symbol << std::endl;

//    for(i=0;i<8;i++)
//    {
//        nine ^= (tmp & 0x01);
//        tmp = tmp >> 1;
//    }

//    if (nine)//(false)//(nine)
//    {
//        std::cout << "set 9 bit" << std::endl;
//        newtio0.c_cflag = (newtio0.c_cflag | PARENB) & (~PARODD);
//    }
//    else
//    {
//        std::cout << "not set 9 bit" << std::endl;
//        newtio0.c_cflag =  newtio0.c_cflag | PARENB | PARODD;
//    }

//    tcdrain(_channelId);

//    if (tcsetattr(_channelId, TCSADRAIN , &newtio0) == -1)
//        return -1;

//    rc = ::write(_channelId, &symbol, 1);


//    //if(!params->get9thBit())
//    //    newtio0.c_cflag = newtio0.c_cflag & (~PARENB) & (~PARODD);

//    //tcdrain(_channelId);

//    {
//        //std::this_thread::sleep_for(chrono::milliseconds(100));
//        // Set receive with space parity
//        newtio0.c_cflag |= PARENB;
//        //newtio0.c_cflag |= CMSPAR;
//        newtio0.c_cflag &= ~PARODD;
//    }

//    //tcsetattr(_channelId, TCSADRAIN, &newtio0);
//    tcsetattr (_channelId, TCSANOW, &newtio0);


    return rc;
}

