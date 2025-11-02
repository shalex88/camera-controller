#include "FpgaTransport.h"

namespace camera_service::infrastructure {
    void FpgaTransport::Read(void* buf, int64_t addr, int64_t len) {
        // cxp_gen_read(addr, buf, len);   //TODO: implement custom GenCP
    }

    void FpgaTransport::Write(const void* buf, int64_t addr, int64_t len) {
        // cxp_gen_write(addr, buf, len);  //TODO: implement custom GenCP
    }

    GenApi_3_5::EAccessMode FpgaTransport::GetAccessMode() const {
        return GenApi_3_5::RW;
    }
}