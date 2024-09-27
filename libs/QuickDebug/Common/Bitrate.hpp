#include <cassert>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <sstream>

namespace QD
{
   // Enum for specifying the format
enum class BitRateFormat {
    Auto,
    Bits,
    bps,
    Kbps,
    Mbps,
    Bytes,
    Bps,
    KBps,
    MBps
};

// Helper function to group digits in a number by inserting apostrophes every 3 digits
std::string GroupDigits(uint64_t value) {
    std::string numStr = std::to_string(value);
    int insertPosition = numStr.length() - 3;

    while (insertPosition > 0) {
        numStr.insert(insertPosition, "'");
        insertPosition -= 3;
    }
    return numStr;
}

void ToBiggestUnit(uint64_t input, BitRateFormat inputFormat, double& output, BitRateFormat& outputFormat) {
    assert(inputFormat == BitRateFormat::Bits || inputFormat == BitRateFormat::Bytes);

    auto unit = 0;
    output = static_cast<double>(input);
    while (output > 1000)
    {
        output /= 1000;
        unit++;
    }

    outputFormat = static_cast<BitRateFormat>(static_cast<uint32_t>(inputFormat) + unit + 1);
}

class BitRate {
public:
    // Constructors accepting various types
    explicit BitRate(const uint64_t bps) : bps_(bps) {}
    explicit BitRate(const uint32_t bps) : bps_(static_cast<uint64_t>(bps)) {}
    explicit BitRate(const int32_t bps) : bps_(static_cast<uint64_t>(bps)) {}
    explicit BitRate(const float bps) : bps_(static_cast<uint64_t>(bps)) {}
    explicit BitRate(const double bps) : bps_(static_cast<uint64_t>(bps)) {}
    explicit BitRate(const uint8_t bps) : bps_(static_cast<uint64_t>(bps)) {}

    // Factory methods for convenience
    static BitRate bps(const uint64_t bps) { return BitRate(bps ); }
    static BitRate Kbps(const uint64_t kbps) { return BitRate(kbps * 1000); }
    static BitRate Mbps(const uint64_t mbps) { return BitRate(mbps * 1000000); }
    static BitRate Bps(const uint64_t Bps) { return BitRate(Bps * 8); }
    static BitRate KBps(const uint64_t KBps) { return BitRate(KBps * 8 * 1000); }
    static BitRate MBps(const uint64_t MBps) { return BitRate(MBps * 8 * 1000000); }

    static BitRate bps(const uint32_t bps) { return BitRate(bps ); }
    static BitRate Kbps(const uint32_t kbps) { return BitRate(kbps * 1000); }
    static BitRate Mbps(const uint32_t mbps) { return BitRate(mbps * 1000000); }
    static BitRate Bps(const uint32_t Bps) { return BitRate(Bps * 8); }
    static BitRate KBps(const uint32_t KBps) { return BitRate(KBps * 8 * 1000); }
    static BitRate MBps(const uint32_t MBps) { return BitRate(MBps * 8 * 1000000); }

    static BitRate bps(const int bps) { return BitRate(bps ); }
    static BitRate Kbps(const int kbps) { return BitRate(kbps * 1000); }
    static BitRate Mbps(const int mbps) { return BitRate(mbps * 1000000); }
    static BitRate Bps(const int Bps) { return BitRate(Bps * 8); }
    static BitRate KBps(const int KBps) { return BitRate(KBps * 8 * 1000); }
    static BitRate MBps(const int MBps) { return BitRate(MBps * 8 * 1000000); }

    // Overloaded factory methods for other data types
    static BitRate Kbps(const float kbps) { return BitRate(static_cast<uint64_t>(kbps * 1000)); }
    static BitRate Mbps(const float mbps) { return BitRate(static_cast<uint64_t>(mbps * 1000000)); }
    static BitRate Kbps(const double kbps) { return BitRate(static_cast<uint64_t>(kbps * 1000)); }
    static BitRate Mbps(const double mbps) { return BitRate(static_cast<uint64_t>(mbps * 1000000)); }

    // Conversion methods
    uint64_t bps() const { return bps_; }
    uint64_t Bps() const { return bps_ / 8; }
    uint64_t Kbps() const { return bps_ / 1000; }
    uint64_t Mbps() const { return bps_ / 1000000; }
    uint64_t KBps() const { return bps_ / (8 * 1000); }
    uint64_t MBps() const { return bps_ / (8 * 1000000); }

    static const char* ToFormatString(const BitRateFormat format)
    {
        switch (format)
        {
        case BitRateFormat::bps:
            return "bps";
        case BitRateFormat::Kbps:
            return "Kbps";
        case BitRateFormat::Mbps:
            return "Mbps";
        case BitRateFormat::Bps:
            return "Bps";
        case BitRateFormat::KBps:
            return "KBps";
        case BitRateFormat::MBps:
            return "MBps";
        default:
            return "";
        }

    }

    // Pretty print the bitrate
    [[nodiscard]] std::string PrettyPrint(BitRateFormat format = BitRateFormat::Auto) const {
        std::ostringstream oss;

        switch (format) {
        case BitRateFormat::bps:
            oss << GroupDigits(bps_) << " bps";
            break;
        case BitRateFormat::Kbps:
            oss << GroupDigits(Kbps()) << " kbps";
            break;
        case BitRateFormat::Mbps:
            oss << GroupDigits(Mbps()) << " Mbps";
            break;
        case BitRateFormat::Bps:
            oss << GroupDigits(Bps()) << " Bps";
            break;
        case BitRateFormat::KBps:
            oss << GroupDigits(KBps()) << " KBps";
            break;
        case BitRateFormat::MBps:
            oss << GroupDigits(MBps()) << " MBps";
            break;
        case BitRateFormat::Bits:
        case BitRateFormat::Bytes:
            double output;
            BitRateFormat outputFormat;
            ToBiggestUnit(format == BitRateFormat::Bits ? bps() : Bps(), format,output, outputFormat);
            oss << output << " " << ToFormatString(outputFormat);
            break;
        case BitRateFormat::Auto:
        default:
            // Automatically select the best format based on magnitude
            if (bps_ >= 8000000) {
                oss << GroupDigits(MBps()) << " MBps";
            } else if (bps_ >= 8000) {
                oss << GroupDigits(KBps()) << " KBps";
            } else if (bps_ >= 1000000) {
                oss << GroupDigits(Mbps()) << " Mbps";
            } else if (bps_ >= 1000) {
                oss << GroupDigits(Kbps()) << " kbps";
            } else {
                oss << GroupDigits(bps_) << " bps";
            }
            break;
        }
        return oss.str();
    }

private:
    uint64_t bps_; // Store the bitrate in bits per second
};
}

