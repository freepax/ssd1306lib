#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <string.h>

#include "ssd1306.h"


/**
 * @brief SSD1306::SSD1306
 *
 * ctor
 *
 * @param device                    i2c device (eg. /dev/i2c-x)
 * @param address                   i2c device address
 */
SSD1306::SSD1306(char *device, unsigned char address) : Firmware_I2C(device, address) {}


/**
 * @brief SSD1306::setAddress
 *
 * communicate the i2c address to the i2c driver internally
 *
 * @param address           i2c address
 *
 * @return                  zero if address is valid, -1 if not
 */
int SSD1306::setAddress(unsigned char address)
{
    if (address != SSD1306Addresses::SSD1306Address0 && address != SSD1306Addresses::SSD1306Address1) {
        std::cerr << __func__ << ":" << __LINE__ << " address given " << address << " is not a valid SSD1306 address" << std::endl;
        std::cerr << __func__ << ":" << __LINE__ << " valid addresses are 0b01111000 or 0b01111010 " << std::endl;
        return -1;
    }

    if (mDebug)
        std::cout << "SSD1306::" << __func__ << ":" << __LINE__ << " hex address x0" << std::hex << address << std::dec << std::endl;

    mAddress = address;

    return 0;
}


/**
 * @brief SSD1306::setScroll
 *
 * @param scroll
 * @param startPage
 * @param endPage
 * @param timeInterval
 * @param offset
 *
 * @return                          zero on success, negative error value on failure
 */
int SSD1306::setScroll(unsigned char scroll, unsigned char startPage, unsigned char endPage, unsigned char timeInterval, unsigned char offset)
{
    /// populate buffer to write
    unsigned char buffer[] = { scroll,  0x00, startPage, timeInterval, endPage, offset };

    /// make sure scroll is not active when scroll settings are updated
    int status = runCommand(Ssd1306DeactivateScroll);
    if (status < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed with status " << status << std::endl;
        return -1;
    }

    /// write six bytes
    status = write(mFd, buffer, 6);
    if (status != 6) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " write failed with status " << status << std::endl;
        return -2;
    }

    /// activate scroll
    status = runCommand(Ssd1306ActivateScroll);
    if (status < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed with status " << status << std::endl;
        return -3;
    }

    return 0;
}


/**
 * @brief SSD1306::runCommand
 *
 * @param command                   send (controll) command to i2c display
 *
 * @return                          zero on success, negative error value on failure
 */
int SSD1306::runCommand(unsigned char command)
{
    unsigned char buffer[] = { 0b00000000, command };

    if (mDebug) {
        std::cerr << "SSD1306::" << __func__ << ":" << __LINE__  << "hex buffer 0 and 1 0x" << std::hex << buffer[0] << " 0x" << buffer[1] << std::dec << std::endl;
        std::cerr << std::dec << std::endl;
    }

    /* write command to i2c driver */
    int status = write(mFd, buffer, 2);
    if (status != 2) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " write failed with status " << status << std::endl;
        return -1;
    }

    return 0;
}


/**
 * @brief SSD1306::writeImage
 *
 * write image data to display
 *
 * @param data                      Image data to display
 *
 * @return                          zero on success, negative error value on failure
 */
int SSD1306::writeImage(unsigned char data[Ssd1306LcdWitdh * SSD1306LcdPages])
{
    /// set column from first to last
    if (runCommand(ssd1306ColumnAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -1;
    }

    if (runCommand(0) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -2;
    }

    if (runCommand(Ssd1306LcdWitdh - 1) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -3;
    }

    /// set page - from first to last
    if (runCommand(ssd1306PageAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -4;
    }

    if (runCommand(0) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -5;
    }

    if (runCommand(SSD1306LcdPages - 1) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -6;
    }

    /// define and prepare buffer
    unsigned char buffer[Ssd1306LcdWitdh * SSD1306LcdPages + 1];
    memset(buffer, 0, Ssd1306LcdWitdh * SSD1306LcdPages + 1);

    /// first byte is write command
    buffer[0] = 0x40;

    /// copy in image bytes - 1024 of them
    for (int i = 1; i < Ssd1306LcdWitdh * SSD1306LcdPages; i++)
        buffer[i] = data[i-1];

    /// write the image date to the ssd1306 device
    if (write(mFd, buffer, Ssd1306LcdWitdh * SSD1306LcdPages + 1) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " write failed" << std::endl;
        return -7;
    }

    return 0;
}


/**
 * @brief SSD1306::writeLine
 *
 * write a single line (string) on display
 *
 * @param page                      page to write
 * @param data                      data (string) to be written
 *
 * @return                          zero on success, negative error value on failure
 */
int SSD1306::writeLine(unsigned char page, unsigned char data[25])
{
    /// set column
    if (runCommand(ssd1306ColumnAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -1;
    }

    if (runCommand(0) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -2;
    }

    if (runCommand(Ssd1306LcdWitdh-1) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -3;
    }

    /// set page - set from/to the same
    if (runCommand(ssd1306PageAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -4;
    }

    if (runCommand(page) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -5;
    }

    if (runCommand(page) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -6;
    }

    unsigned char buffer[Ssd1306LcdWitdh + 1];
    memset(buffer, 0, Ssd1306LcdWitdh + 1);

    /// first byte is write command
    buffer[0] = 0x40;

    int offset = 0;
    int f = 0;

    /// data follows - look up bytes in the font array
    for (int i = 1; i < 127; i++) {
        if (f == 5) {
            offset++;
            f = 0;
        }
        if (i < 125)
            buffer[i] = font[data[offset] * 5 + f++];
    }

    if (write(mFd, buffer, Ssd1306LcdWitdh + 1) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " write failed" << std::endl;
        return -7;
    }

    return 0;
}


/**
 * @brief SSD1306::writeByte
 *
 * write a sigle byte (character) to display
 *
 * @param line                      line to be written
 * @param position                  position in line to be written
 * @param data                      data to be written
 *
 * @return                          zero on success, negative error value on failure
 */
int SSD1306::writeByte(unsigned char line, unsigned char position, unsigned char data)
{
    /// set column - set from/to the same
    if (runCommand(ssd1306ColumnAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -1;
    }

    if (runCommand(position) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -2;
    }

    if (runCommand(position) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -3;
    }

    /// set page - set from/to the same
    if (runCommand(ssd1306PageAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -4;
    }

    if (runCommand(line) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -5;
    }

    if (runCommand(line) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -6;
    }

    unsigned char buffer[] = { 0x40, data };

    if (write(mFd, buffer, 2) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " write failed" << std::endl;
        return -7;
    }

    return 0;
}


/**
 * @brief SSD1306::clearLine
 *
 * Clean line on display
 *
 * @param line                      line to clean
 *
 * @return                          zero on success, negative error value on failure
 */
int SSD1306::clearLine(int line)
{
    /// set column
    if (runCommand(ssd1306ColumnAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -1;
    }

    if (runCommand(0) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -2;
    }

    if (runCommand(Ssd1306LcdWitdh - 1) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -3;
    }

    /// set page - set from/to equal
    if (runCommand(ssd1306PageAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -4;
    }

    if (runCommand(line) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -5;
    }

    if (runCommand(line) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -6;
    }

    unsigned char data[128];
    memset(data, 0, 128);
    data[0] = 0x40;

    /// write 65 bytes
    if (write(mFd, data, 129) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " write failed" << std::endl;
        return -7;
    }

    return 0;
}


/**
 * @brief SSD1306::clearDisplay
 *
 * clear entire display
 *
 * @return                          zero on success, negative error value on failure
 */
int SSD1306::clearDisplay()
{
    /// set column
    if (runCommand(ssd1306ColumnAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -1;
    }

    if (runCommand(0) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -2;
    }

    if (runCommand(Ssd1306LcdWitdh - 1) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -3;
    }

    /// set page
    if (runCommand(ssd1306PageAddress) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -4;
    }

    if (runCommand(0) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -5;
    }

    if (runCommand(SSD1306LcdPages - 1) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " runCommand failed" << std::endl;
        return -6;
    }

    unsigned char data[1025];
    memset(data, 0, 1025);
    data[0] = 0x40;

    /// write 1025 bytes
    if (write(mFd, data, 1025) < 0) {
        std::cerr << "SSD1306::"  << __func__ << ":" << __LINE__ << " write failed" << std::endl;
        return -7;
    }

    return 0;
}
