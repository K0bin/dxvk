#include "d3d9_const_ring_buffer.h"

#include <windef.h>

#include "d3d9_device.h"

#include "../util/util_math.h"


namespace dxvk {

  D3D9ConstRingBuffer::D3D9ConstRingBuffer(uint32_t Size)
    : m_size(Size) {
    m_data = new (std::align_val_t(64)) uint8_t[Size];
    std::memset(m_data, 0, Size);
  }

  D3D9ConstRingBuffer::~D3D9ConstRingBuffer() {
    delete m_data;
  }

  template<typename T>
  const T* D3D9ConstRingBuffer::Push(D3D9DeviceEx* Device, const T* pData, uint32_t ElementCount) {
    uint32_t dataSize = sizeof(T) * ElementCount;
    if (unlikely(dataSize > m_size || dataSize == 0)) {
      // we're screwed
      Logger::err("Data doesnt fit or is zero sized.");
      return nullptr;
    }

    uint32_t dataOffset = 0;
    uint32_t freeSpace = 0;
    uint64_t seqNum = Device->GetCurrentSequenceNumber();

    uint32_t entryPos = 0;
    if (likely(!m_entries.empty())) {
      {
        const auto& previousEntry = m_entries[m_previousEntry];
        dataOffset = align(previousEntry.offset + previousEntry.size, std::alignment_of<T>::value);
      }

      entryPos = m_previousEntry + 1;
      auto entryIter = m_entries.begin() + entryPos;
      if (likely(entryIter != m_entries.end())) {

        while (freeSpace < dataSize && entryIter != m_entries.end()) {
          Device->SynchronizeCsThread(entryIter->seqNum);
          freeSpace = (entryIter->offset - dataOffset) + entryIter->size;
          m_entries.erase(entryIter);
          entryIter = m_entries.begin() + entryPos;
        }
      } else {
        freeSpace = m_size - dataOffset;
      }

      auto& previousEntry = m_entries[m_previousEntry];
      if (likely(previousEntry.seqNum == seqNum && freeSpace >= dataSize)) {
        // The current entry has the the same sequence number as the current one
        // => Try to append the data instead of adding a new entry.
        previousEntry.size = (dataOffset + dataSize) - previousEntry.offset;
        entryPos = m_previousEntry;
      }
    } else {
      freeSpace = m_size - dataOffset;
    }

    if (likely(freeSpace >= dataSize)) {
      memcpy(m_data + dataOffset, reinterpret_cast<const uint8_t*>(pData), dataSize);
      if (unlikely(entryPos != m_previousEntry || m_entries.empty())) {
        auto entryIter = m_entries.begin() + entryPos;
        m_entries.insert(entryIter, {
          dataOffset,
          dataSize,
          seqNum
        });
        m_previousEntry = entryPos;
      }
      return reinterpret_cast<T*>(m_data + dataOffset);
    }

    freeSpace = 0;
    m_previousEntry = 0;
    while (freeSpace < dataSize) {
      const auto& entry = m_entries.front();
      Device->SynchronizeCsThread(entry.seqNum);
      freeSpace += entry.size;
      m_entries.pop_front();
    }
    memcpy(m_data, reinterpret_cast<const uint8_t*>(pData), dataSize);
    m_entries.push_front({
      0,
      dataSize,
      seqNum
    });
    return reinterpret_cast<T*>(m_data);
  }

  template const float* D3D9ConstRingBuffer::Push<float>(D3D9DeviceEx* Device, const float* Data, uint32_t ElementCount);
  template const INT* D3D9ConstRingBuffer::Push<INT>(D3D9DeviceEx* Device, const INT* Data, uint32_t ElementCount);
  // BOOL == INT

}