// PTP pass-through using the installed Windows WPD driver; no USB driver swap.
// stdin: read|none OPCODE [PARAM ...], or write OPCODE PARAM ... | HEXDATA
// Numbers are hexadecimal. stdout is one JSON object per line.
#define NOMINMAX
#include <windows.h>
#include <initguid.h>
#include <PortableDeviceApi.h>
#include <PortableDevice.h>
#include <WpdMtpExtensions.h>
#include <propvarutil.h>
#include <wrl/client.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
using Microsoft::WRL::ComPtr;

void check(HRESULT hr, const char* where) {
    if (FAILED(hr)) {
        std::ostringstream s; s << where << " HRESULT=0x" << std::hex << (unsigned long)hr;
        throw std::runtime_error(s.str());
    }
}
std::string utf8(const wchar_t* s) {
    if (!s) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
    std::string r(n, '\0'); WideCharToMultiByte(CP_UTF8, 0, s, -1, r.data(), n, nullptr, nullptr);
    r.resize(n - 1); return r;
}
std::string quote(const std::string& s) {
    std::string r = "\"";
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') { r += '\\'; r += c; }
        else if (c < 32) r += '?'; else r += c;
    }
    return r + "\"";
}
ComPtr<IPortableDeviceValues> values() {
    ComPtr<IPortableDeviceValues> p;
    check(CoCreateInstance(CLSID_PortableDeviceValues, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&p)), "create values");
    return p;
}
ComPtr<IPortableDeviceValues> command(REFPROPERTYKEY key) {
    auto p = values();
    check(p->SetGuidValue(WPD_PROPERTY_COMMON_COMMAND_CATEGORY, key.fmtid), "category");
    check(p->SetUnsignedIntegerValue(WPD_PROPERTY_COMMON_COMMAND_ID, key.pid), "command ID");
    return p;
}
ComPtr<IPortableDeviceValues> send(IPortableDevice* dev, IPortableDeviceValues* p) {
    ComPtr<IPortableDeviceValues> r;
    check(dev->SendCommand(0, p, &r), "SendCommand");
    HRESULT hr = S_OK;
    if (SUCCEEDED(r->GetErrorValue(WPD_PROPERTY_COMMON_HRESULT, &hr))) check(hr, "WPD command");
    return r;
}
std::string hex(const std::vector<BYTE>& b) {
    std::ostringstream s; s << std::hex << std::setfill('0');
    for (BYTE x : b) s << std::setw(2) << (unsigned)x;
    return s.str();
}

int main() {
    try {
        check(CoInitializeEx(nullptr, COINIT_MULTITHREADED), "COM init");
        ComPtr<IPortableDeviceManager> mgr;
        check(CoCreateInstance(CLSID_PortableDeviceManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mgr)), "device manager");
        DWORD n = 0; check(mgr->GetDevices(nullptr, &n), "device count");
        std::vector<LPWSTR> ids(n); check(mgr->GetDevices(ids.data(), &n), "device IDs");
        std::wstring target;
        for (DWORD i = 0; i < n; i++) {
            wchar_t name[512] = {}; DWORD size = 512;
            mgr->GetDeviceFriendlyName(ids[i], name, &size);
            std::cout << "{\"device\":" << quote(utf8(name)) << "}" << std::endl;
            if (std::wstring(name).find(L"ILCE-1M2") != std::wstring::npos) {
                if (!target.empty()) throw std::runtime_error("Multiple A1 II devices; refusing ambiguous target");
                target = ids[i];
            }
            CoTaskMemFree(ids[i]);
        }
        if (target.empty()) throw std::runtime_error("ILCE-1M2 not found through WPD");
        ComPtr<IPortableDevice> dev;
        check(CoCreateInstance(CLSID_PortableDeviceFTM, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dev)), "create device");
        auto client = values();
        client->SetStringValue(WPD_CLIENT_NAME, L"AlphaFairy Clock Probe");
        client->SetUnsignedIntegerValue(WPD_CLIENT_MAJOR_VERSION, 1);
        client->SetUnsignedIntegerValue(WPD_CLIENT_MINOR_VERSION, 0);
        client->SetUnsignedIntegerValue(WPD_CLIENT_REVISION, 0);
        client->SetUnsignedIntegerValue(WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, SECURITY_IMPERSONATION);
        check(dev->Open(target.c_str(), client.Get()), "open A1 II");
        std::cout << "{\"ready\":true}" << std::endl;
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line == "quit") break;
            try {
                std::istringstream in(line.substr(0, line.find('|')));
                std::string dir; unsigned long op = 0;
                if (!(in >> dir >> std::hex >> op) || op > 0xffff) throw std::runtime_error("invalid opcode");
                if (dir != "read" && dir != "none" && dir != "write") throw std::runtime_error("bad direction");
                std::vector<ULONG> params; unsigned long x;
                while (in >> std::hex >> x) params.push_back(x);
                if (params.size() > 5) throw std::runtime_error("too many parameters");
                std::vector<BYTE> data;
                if (dir == "write") {
                    // Restrict writes to clock experiments. No capture, erase or firmware ops.
                    bool clockProp = params.size() == 1 && (params[0] == 0x5011 || params[0] == 0xd223);
                    if (!((clockProp && (op == 0x1016 || op == 0x9205 || op == 0x9207)) || (op == 0x9249 && params.empty())))
                        throw std::runtime_error("write is not an allowed clock operation");
                    auto sep = line.find('|');
                    if (sep == std::string::npos) throw std::runtime_error("missing payload");
                    std::string h;
                    for (char c : line.substr(sep + 1)) if (!isspace((unsigned char)c)) h += c;
                    if (h.size() % 2 || h.size() > 4096) throw std::runtime_error("invalid payload length");
                    for (size_t i = 0; i < h.size(); i += 2) data.push_back((BYTE)std::stoul(h.substr(i, 2), nullptr, 16));
                }
                auto p = command(dir == "read" ? WPD_COMMAND_MTP_EXT_EXECUTE_COMMAND_WITH_DATA_TO_READ :
                                 dir == "write" ? WPD_COMMAND_MTP_EXT_EXECUTE_COMMAND_WITH_DATA_TO_WRITE :
                                                  WPD_COMMAND_MTP_EXT_EXECUTE_COMMAND_WITHOUT_DATA_PHASE);
                p->SetUnsignedIntegerValue(WPD_PROPERTY_MTP_EXT_OPERATION_CODE, op);
                ComPtr<IPortableDevicePropVariantCollection> pp;
                check(CoCreateInstance(CLSID_PortableDevicePropVariantCollection, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pp)), "params");
                for (ULONG v : params) { PROPVARIANT pv; PropVariantInit(&pv); pv.vt = VT_UI4; pv.ulVal = v; check(pp->Add(&pv), "add param"); }
                p->SetIPortableDevicePropVariantCollectionValue(WPD_PROPERTY_MTP_EXT_OPERATION_PARAMS, pp.Get());
                if (dir == "write") p->SetUnsignedLargeIntegerValue(WPD_PROPERTY_MTP_EXT_TRANSFER_TOTAL_DATA_SIZE, data.size());
                auto r = send(dev.Get(), p.Get());
                LPWSTR rawCtx = nullptr;
                ULONG response = 0; r->GetUnsignedIntegerValue(WPD_PROPERTY_MTP_EXT_RESPONSE_CODE, &response);
                if (dir != "none" && SUCCEEDED(r->GetStringValue(WPD_PROPERTY_MTP_EXT_TRANSFER_CONTEXT, &rawCtx))) {
                    std::wstring ctx(rawCtx); CoTaskMemFree(rawCtx);
                    try {
                        if (dir == "read") {
                            ULONGLONG total = 0; check(r->GetUnsignedLargeIntegerValue(WPD_PROPERTY_MTP_EXT_TRANSFER_TOTAL_DATA_SIZE, &total), "data size");
                            if (total > 8 * 1024 * 1024) throw std::runtime_error("read exceeds 8MB safety cap");
                            data.clear();
                            while (data.size() < total) {
                                ULONG count = (ULONG)std::min<ULONGLONG>(65536, total - data.size());
                                std::vector<BYTE> buf(count);
                                auto q = command(WPD_COMMAND_MTP_EXT_READ_DATA);
                                q->SetStringValue(WPD_PROPERTY_MTP_EXT_TRANSFER_CONTEXT, ctx.c_str());
                                q->SetUnsignedIntegerValue(WPD_PROPERTY_MTP_EXT_TRANSFER_NUM_BYTES_TO_READ, count);
                                q->SetBufferValue(WPD_PROPERTY_MTP_EXT_TRANSFER_DATA, buf.data(), count);
                                auto rr = send(dev.Get(), q.Get());
                                BYTE* b = nullptr; DWORD nb = 0;
                                check(rr->GetBufferValue(WPD_PROPERTY_MTP_EXT_TRANSFER_DATA, &b, &nb), "read buffer");
                                data.insert(data.end(), b, b + nb); CoTaskMemFree(b);
                                if (nb == 0) break;
                            }
                        } else {
                            auto q = command(WPD_COMMAND_MTP_EXT_WRITE_DATA);
                            q->SetStringValue(WPD_PROPERTY_MTP_EXT_TRANSFER_CONTEXT, ctx.c_str());
                            q->SetUnsignedIntegerValue(WPD_PROPERTY_MTP_EXT_TRANSFER_NUM_BYTES_TO_WRITE, (ULONG)data.size());
                            q->SetBufferValue(WPD_PROPERTY_MTP_EXT_TRANSFER_DATA, data.data(), (DWORD)data.size());
                            auto rr = send(dev.Get(), q.Get());
                            ULONG written = 0; check(rr->GetUnsignedIntegerValue(WPD_PROPERTY_MTP_EXT_TRANSFER_NUM_BYTES_WRITTEN, &written), "write count");
                            if (written != data.size()) throw std::runtime_error("short write");
                        }
                    } catch (...) {
                        auto end = command(WPD_COMMAND_MTP_EXT_END_DATA_TRANSFER);
                        end->SetStringValue(WPD_PROPERTY_MTP_EXT_TRANSFER_CONTEXT, ctx.c_str());
                        try { send(dev.Get(), end.Get()); } catch (...) {}
                        throw;
                    }
                    auto end = command(WPD_COMMAND_MTP_EXT_END_DATA_TRANSFER);
                    end->SetStringValue(WPD_PROPERTY_MTP_EXT_TRANSFER_CONTEXT, ctx.c_str());
                    r = send(dev.Get(), end.Get());
                    check(r->GetUnsignedIntegerValue(WPD_PROPERTY_MTP_EXT_RESPONSE_CODE, &response), "PTP response");
                }
                std::cout << "{\"request\":" << quote(line) << ",\"response\":" << response
                          << ",\"size\":" << data.size() << ",\"data\":" << quote(hex(data)) << "}" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "{\"request\":" << quote(line) << ",\"error\":" << quote(e.what()) << "}" << std::endl;
            }
        }
        dev->Close();
    } catch (const std::exception& e) { std::cout << "{\"fatal\":" << quote(e.what()) << "}" << std::endl; return 1; }
    return 0;
}
