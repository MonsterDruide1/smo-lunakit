#include <lib.hpp>
#include "FunctionHelper.h"
#include "nn/ro.h"
#include <armv8/instructions/opx1x0/load_store_register_unsigned_immediate/base.hpp>

namespace inst = exl::armv8::inst;
namespace reg = exl::armv8::reg;

static constexpr const u32 RetValue = exl::armv8::inst::Ret().Value();

uintptr_t FunctionHelper::findEndOfFunc(const char* symbol) {
    u32* ptr = (u32*)getAddressFromSymbol(symbol);
    // naive implementation, ignores the fact that functions can have multiple returns
    while (*ptr != RetValue) {
        ptr++;
    }
    return (uintptr_t) ptr;
}

ptrdiff_t FunctionHelper::readLdrOffset(const char* symbol) {
    auto ldr = *reinterpret_cast<inst::LdrRegisterImmediate*>(getAddressFromSymbol(symbol));
    auto rnReg = reg::Register(static_cast<reg::RegisterKind>(((exl::armv8::inst::impl::opx1x0::LoadStoreRegisterUnsignedImmediate)ldr).GetSize() & 0b01), ldr.GetRn());
    return ldr.GetImm12() * (rnReg.Is64() ? sizeof(uint64_t) : sizeof(uint32_t));

}
