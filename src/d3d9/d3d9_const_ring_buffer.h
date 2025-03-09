#pragma once

#include <cstdint>
#include <deque>

namespace dxvk {

  struct D3D9ConstRingBufferEntry {
    uint32_t offset;
    uint32_t size;
    uint64_t seqNum;
  };

  class D3D9DeviceEx;

  class D3D9ConstRingBuffer {

    public:

    D3D9ConstRingBuffer(uint32_t Size);

    ~D3D9ConstRingBuffer();

    template<typename T>
    const T* Push(D3D9DeviceEx* Device, const T* pData, uint32_t ElementCount);

    void SetPreviousEntrySeqNum(uint64_t seqNum) {
      m_entries[m_previousEntry].seqNum = seqNum;
    }

    private:

    uint32_t m_size;
    uint8_t* m_data;

    uint32_t m_previousEntry = 0;
    std::deque<D3D9ConstRingBufferEntry> m_entries;

  };

}
