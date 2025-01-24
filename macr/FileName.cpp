#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "resource.h"
#include <fstream>



using namespace std;

bool loadPngImageFromResource(int resourceId, cv::Mat& image) {
    
    HRSRC hResInfo = FindResource(NULL, MAKEINTRESOURCE(resourceId), L"PNG");
    if (!hResInfo) {
        DWORD error = GetLastError();  
        std::cerr << "No res! Code: " << error << std::endl;
        return false;
    }

    DWORD dwSize = SizeofResource(NULL, hResInfo);
    HGLOBAL hResData = LoadResource(NULL, hResInfo);
    if (!hResData) {
        DWORD error = GetLastError();  
        std::cerr << "Не удалось загрузить ресурс Код ошибки: " << error << std::endl;
        return false;
    }

    void* pData = LockResource(hResData); 
    if (!pData) {
        std::cerr << "Не удалось заблокировать ресурс" << std::endl;
        return false;
    }


    std::vector<uchar> data((uchar*)pData, (uchar*)pData + dwSize);

    image = cv::imdecode(data, cv::IMREAD_GRAYSCALE);
    if (image.empty()) {
        std::cerr << "Не удалось загрузить изображение из ресурса" << std::endl;
        return false;
    }

    return true;
}



bool running = false;


bool compareImages(const cv::Mat& inputImage, const cv::Mat& templateImage)
{

    cv::Mat img1 = inputImage;
    cv::Mat img2 = templateImage;

    if (img1.depth() == CV_16U) {
        img1.convertTo(img1, CV_8U, 1.0 / 256); 
    }

    if (img2.channels() == 1) {
        cv::cvtColor(img2, img2, cv::COLOR_GRAY2RGB);
    }

    if (img1.channels() != 3) {
        cv::cvtColor(img1, img1, cv::COLOR_GRAY2RGB);  
    }

    if (img2.channels() != 3) {
        cv::cvtColor(img2, img2, cv::COLOR_GRAY2RGB);  
    }

    cv::Mat result;
    
    cv::matchTemplate(img1, img2, result, cv::TM_CCOEFF_NORMED);

    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

    if (maxVal > 0.8) {
        return true;
    }

    return false;
}
void simulate_click() {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}
void check_key_state() {
    while (true) {
        if (GetAsyncKeyState(VK_XBUTTON2)) {
            running = !running;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }
}





using namespace cv;
Mat hwnd2mat(HWND hwnd)
{
    HDC hwindowDC, hwindowCompatibleDC;

    int height, width, srcheight, srcwidth;
    HBITMAP hbwindow;
    Mat src;
    BITMAPINFOHEADER bi;

   
    hwindowDC = GetDC(hwnd);
    hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

    RECT windowsize;
    GetClientRect(hwnd, &windowsize);

    srcheight = windowsize.bottom;
    srcwidth = windowsize.right;
    height = srcheight;  
    width = srcwidth;

    src.create(height, width, CV_8UC4);

    
    hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

  
    SelectObject(hwindowCompatibleDC, hbwindow);

   
    StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY);
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

  
    DeleteObject(hbwindow);
    DeleteDC(hwindowCompatibleDC);
    ReleaseDC(hwnd, hwindowDC);

   
    cv::Mat rgbSrc;
    cv::cvtColor(src, rgbSrc, cv::COLOR_RGBA2RGB); 

    return rgbSrc; 
}
cv::Mat captureTopRight(HWND hwnd)
{
  
    cv::Mat fullImage = hwnd2mat(hwnd);

    int screenWidth = fullImage.cols;  
    int screenHeight = fullImage.rows; 

  
    int height = 74;
    int width = 70;

    float topOffsetPercent = 27.0f / 1440.0f;   
    float rightOffsetPercent = 62.0f / 2560.0f; 

    int topOffset = static_cast<int>(screenHeight * topOffsetPercent);
    int rightOffset = static_cast<int>(screenWidth * rightOffsetPercent);


    int x = screenWidth - width - rightOffset;
    int y = topOffset;

    cv::Rect roi(x, y, width, height);

    cv::Mat croppedImage = fullImage(roi);

    return croppedImage;
}

HWND hwndDesktop = GetDesktopWindow();
cv::Mat templateImage;
void compare_and_output() {
    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        while (running) {

            if (compareImages(captureTopRight(hwndDesktop), templateImage)) {
                //std::cout << "Изображения совпадают" << std::endl;
                running = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            else {
                //std::cout << "Изображения не совпадают" << std::endl;
            }


            std::this_thread::sleep_for(std::chrono::milliseconds(350));
        }
    }
}
int main() {
    setlocale(LC_ALL, "rus");


    
    
    if (!loadPngImageFromResource(IDB_PNG1, templateImage)) {
        std::cerr << "Ошибка загрузки шаблона из ресурсов" << std::endl;
        return -1;
    }
   // std::cout << "templateImage: " << templateImage.type() << std::endl;
   
    std::thread key_thread(check_key_state);
    std::thread compare_thread(compare_and_output);
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        while (running) {
           
            simulate_click();
           
            std::this_thread::sleep_for(std::chrono::milliseconds(50));  

        }
    }
    
}