#include "localserver/HJDownloader.h"
#include <iostream>
#include <string>
#include <iomanip> // For std::fixed and std::setprecision

// A simple progress callback function to print download progress to the console.
void printProgress(int64_t downloaded, int64_t total) {
    if (total <= 0) {
        std::cout << "Downloaded: " << downloaded << " bytes (total size unknown)" << std::endl;
        return;
    }

    double percentage = 100.0 * downloaded / total;
    double downloaded_mb = downloaded / (1024.0 * 1024.0);
    double total_mb = total / (1024.0 * 1024.0);

    // Use carriage return to show progress on a single line
    std::cout << "\r"
              << "Progress: " << std::fixed << std::setprecision(2) << percentage << "% "
              << "(" << downloaded_mb << " MB / " << total_mb << " MB)      "
              << std::flush;
    
    if (downloaded == total) {
        std::cout << std::endl;
    }
}

int main() {
    // Use the hj namespace where HJDownloader is defined
    using namespace hj;

    // A public URL for a test video (Big Buck Bunny)
    std::string url = "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4";
    
    // The local path to save the file
    std::string filepath = "BigBuckBunny.mp4";

    std::cout << "Starting download..." << std::endl;
    std::cout << "URL: " << url << std::endl;
    std::cout << "Output file: " << filepath << std::endl;

    // Create the downloader instance
    auto downloader = HJDownloader::create(url, filepath);

    // Set the callback function
    downloader->setProgressCallback(printProgress);

    // Start the download
    if (downloader->start()) {
        std::cout << "Download completed successfully!" << std::endl;
    } else {
        std::cerr << "\nDownload failed. Error: " << downloader->getLastError() << std::endl;
        return 1; // Return an error code
    }

    return 0;
}
