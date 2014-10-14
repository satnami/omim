#pragma once

#include "../coding/file_container.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/unordered_map.hpp"
#include "../std/utility.hpp"

#include "../3party/succinct/elias_fano_compressed_list.hpp"


namespace routing
{

typedef uint32_t OsrmNodeIdT;
extern OsrmNodeIdT const INVALID_NODE_ID;

class OsrmFtSegMapping
{
public:

#pragma pack (push, 1)
  struct FtSeg
  {
    uint32_t m_fid;
    uint16_t m_pointStart;
    uint16_t m_pointEnd;

    static const uint32_t INVALID_FID = -1;

    // No need to initialize something her (for vector<FtSeg>).
    FtSeg() {}
    FtSeg(uint32_t fid, uint32_t ps, uint32_t pe);

    explicit FtSeg(uint64_t x);
    uint64_t Store() const;

    bool Merge(FtSeg const & other);

    bool operator == (FtSeg const & other) const
    {
      return (other.m_fid == m_fid)
          && (other.m_pointEnd == m_pointEnd)
          && (other.m_pointStart == m_pointStart);
    }

    bool IsIntersect(FtSeg const & other) const;

    bool IsValid() const
    {
      return m_fid != INVALID_FID;
    }

    friend string DebugPrint(FtSeg const & seg);
  };

  struct SegOffset
  {
    OsrmNodeIdT m_nodeId;
    uint32_t m_offset;

    SegOffset() : m_nodeId(0), m_offset(0) {}

    SegOffset(uint32_t nodeId, uint32_t offset)
      : m_nodeId(nodeId), m_offset(offset)
    {
    }

    friend string DebugPrint(SegOffset const & off);
  };
#pragma pack (pop)

  struct FtSegLess
  {
    bool operator() (FtSeg const * a, FtSeg const * b) const
    {
      return a->Store() < b->Store();
    }
  };

  typedef set<FtSeg*, FtSegLess> FtSegSetT;

  void Clear();
  void Load(FilesMappingContainer & cont);

  void Map(FilesMappingContainer & cont);
  void Unmap();
  bool IsMapped() const;

  template <class ToDo> void ForEachFtSeg(OsrmNodeIdT nodeId, ToDo toDo) const
  {
    pair<size_t, size_t> r = GetSegmentsRange(nodeId);
    while (r.first != r.second)
    {
      FtSeg s(m_segments[r.first]);
      if (s.m_fid != FtSeg::INVALID_FID)
        toDo(s);
      ++r.first;
    }
  }

  typedef unordered_map<uint64_t, pair<OsrmNodeIdT, OsrmNodeIdT> > OsrmNodesT;
  void GetOsrmNodes(FtSegSetT & segments, OsrmNodesT & res, volatile bool const & requestCancel) const;

  /// @name For debug purpose only.
  //@{
  void DumpSegmentsByFID(uint32_t fID) const;
  void DumpSegmentByNode(OsrmNodeIdT nodeId) const;
  //@}

  /// @name For unit test purpose only.
  //@{
  /// @return STL-like range [s, e) of segments indexies for passed node.
  pair<size_t, size_t> GetSegmentsRange(uint32_t nodeId) const;
  /// @return Node id for segment's index.
  OsrmNodeIdT GetNodeId(size_t segInd) const;

  size_t GetSegmentsCount() const { return m_segments.size(); }
  //@}

protected:
  typedef vector<SegOffset> SegOffsetsT;
  SegOffsetsT m_offsets;

private:
  succinct::elias_fano_compressed_list m_segments;
  FilesMappingContainer::Handle m_handle;
};


class OsrmFtSegMappingBuilder : public OsrmFtSegMapping
{
public:
  OsrmFtSegMappingBuilder();

  typedef vector<FtSeg> FtSegVectorT;

  void Append(OsrmNodeIdT nodeId, FtSegVectorT const & data);
  void Save(FilesContainerW & cont) const;

private:
  vector<uint64_t> m_buffer;
  uint64_t m_lastOffset;
};

}
