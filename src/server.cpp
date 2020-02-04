#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fstream>
#include <string>
#include <dirent.h>
#include <vector>

using namespace std;

class open_failure: public exception
{
    virtual const char* what() const throw() {
        return "Device could not be opened";
    }
} open_failure;

class no_device: public exception
{
    virtual const char* what() const throw() {
        return "No device found";
    }
} no_device;

class info_failure: public exception
{
    virtual const char* what() const throw() {
        return "No device responded";
    }
} info_failure;

std::string processFile(struct dirent *file)
{
    if (file->d_type == DT_CHR)
    {
        if (strstr((const char *)file->d_name, (const char *)"video"))
            return (std::string(file->d_name));
    }
    return "";
}

int access_camera(void)
{
    std::string path = "/dev";
    std::vector<std::string> devs;
    auto dir = opendir(path.c_str());
    if (dir == NULL)
        return (open("/dev/video0", O_RDWR));
    path = path + "/";
    while (auto file = readdir(dir))
    {
        auto filename = processFile(file);
        if (!filename.empty())
            devs.push_back(path + filename);
    }
    closedir(dir);
    if (devs.empty())
        throw no_device;
    std::vector<std::string>::const_iterator dev;
    int fd;
    for (dev = devs.begin(); dev != devs.end(); ++dev)
    {
        fd = open((*dev).c_str(), O_RDWR);
        if (fd < 0)
            continue;
        v4l2_capability ability;
        if (ioctl(fd, VIDIOC_QUERYCAP, &ability) < 0)
            continue;
        cout << "filename: " << (*dev).c_str() << endl;
        return (fd);
    }
    if (fd < 0)
        throw open_failure;
    throw info_failure;
}

int main(int argc, char **argv)
{
    int fd;
    int width = 0;
    int height = 0;
    try
    {
        cout << "Trying to access a webcam..." << endl;
        fd = access_camera();
    }
    catch(exception &e) {
        std::cout << "Error occured:" << e.what() << std::endl;
    }
    cout << "fd is: " << fd << endl;
    if (argc == 3)
    {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }
    if (width <= 0 || height <= 0)
    {
        width = 1024;
        height = 1024;
    }
    v4l2_format imgForm;
    imgForm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imgForm.fmt.pix.width = width;
    imgForm.fmt.pix.height = height;
    imgForm.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    imgForm.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(fd, VIDIOC_S_FMT, &imgForm) < 0)
    {
        perror("Failure in setting image format, VIDIOC_S_FMT");
        exit(1);
    }
    v4l2_requestbuffers requestBuff = {0};
    requestBuff.count = 1;
    requestBuff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuff.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &requestBuff) < 0)
    {
        perror("Could not request buffer from device, VIDIOC_REQBUFS");
        exit(1);
    }

    v4l2_buffer queryBuff = {0};
    queryBuff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queryBuff.memory = V4L2_MEMORY_MMAP;
    queryBuff.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &queryBuff) < 0)
    {
        perror("Device did not return the buffer information, VIDIOC_QUERYBUF");
        exit(1);
    }
    char *buffer = (char *)mmap(NULL, queryBuff.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, queryBuff.m.offset);
    memset(buffer, 0, queryBuff.length);
    v4l2_buffer buffinfo;
    memset(&buffinfo, 0, sizeof(buffinfo));
    buffinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffinfo.memory = V4L2_MEMORY_MMAP;
    buffinfo.index = 0;
    int type = buffinfo.type;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
        perror("Streaming failed, VIDIOC_STREAMON");
        exit(1);
    }
    if (ioctl(fd, VIDIOC_QBUF, &buffinfo) < 0)
    {
        perror("Could not queue buffer, VIDIOC_QBUF");
        exit(1);
    }
    if (ioctl(fd, VIDIOC_DQBUF, &buffinfo) < 0)
    {
        perror("Could not dequeue buffer, VIDIOC_DQBUF");
        exit(1);
    }
    cout << "Buffer has " << (double)buffinfo.bytesused / 1024 << " Kbytes written to it" << endl;
    ofstream outF;
    outF.open("webcam_capture.jpeg", ios::binary | ios::trunc);
    int bufPos = 0, blockSize = 0;
    int remainSize = buffinfo.bytesused;
    char *newMem = NULL;
    int iter = 0;
    while (remainSize > 0)
    {
        bufPos += blockSize;
        blockSize = 1024;
        newMem = new char[sizeof(char) * blockSize];
        memcpy(newMem, buffer + bufPos, blockSize);
        outF.write(newMem, blockSize);
        remainSize -= blockSize;
        if (blockSize > remainSize)
            blockSize = remainSize;
        delete newMem;
    }
    outF.close();
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
    {
        perror("Could not end streaming, VIDIOC_STREAMOFF");
        exit(1);
    }
    close(fd);
    return (0);
}
