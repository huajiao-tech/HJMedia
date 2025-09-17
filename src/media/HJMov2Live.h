//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <map>
#include "HJXIOFile.h"
#include "HJBuffer.h"
#include "HJMediaUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
#define HJ_ATOM_PREAMBLE_SIZE      8
/**
 * atom size; data size
 */
class HJMovAtom : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJMovAtom>;
    HJMovAtom() {}
    HJMovAtom(uint32_t type, size_t size, size_t offset)
        : m_type(type)
        , m_size(size)
        , m_offset(offset) { }
    
    void setData(const HJBuffer::Ptr& data) {
        m_data = data;
    }
    const HJBuffer::Ptr& getData() {
        return m_data;
    }
    const uint8_t* getDataPtr() {
        if (m_data) {
            return m_data->data();
        }
        return NULL;
    }
    const size_t getDataSize() {
        if (m_data) {
            return m_data->size();
        }
        return 0;
    }
    void writeData(uint8_t* data, int64_t size) {
        if (!data || size <= 0) {
            return;
        }
        if (!m_data) {
            m_data = std::make_shared<HJBuffer>(size);
        }
        m_data->appendData(data, size);
    }
    const uint32_t getType() {
        return m_type;
    }
    void setType(const uint32_t type) {
        m_type = type;
    }
    const size_t getSize() {
        return m_size;
    }
    void setSize(const size_t size) {
        m_size = size;
    }
    const size_t getOffset() {
        return m_offset;
    }
    void setOffset(const size_t offset) {
        m_offset = offset;
    }
    void clone(const HJMovAtom::Ptr& other) {
        m_type = other->m_type;
        m_size = other->m_size;
        m_offset = other->m_offset;
    }
    HJMovAtom::Ptr dup() {
        HJMovAtom::Ptr atom = std::make_shared<HJMovAtom>(m_type, m_size, m_offset);
        return atom;
    }
    void setAtEnd(const bool atEnd) {
        m_atEnd = atEnd;
    }
    const bool getAtEnd() {
        return m_atEnd;
    }
protected:
    uint32_t        m_type = 0;
    size_t          m_size = 0;
    size_t          m_offset = 0;
    HJBuffer::Ptr  m_data = nullptr;
    bool            m_atEnd = true;
};
using HJMovAtomList = std::list<HJMovAtom::Ptr>;

class HJMovFile : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJMovFile>;
    
    int parse(const HJXIOFile::Ptr& ioFile);

    const HJMovAtom::Ptr& getFtypAtom() {
        return m_ftypAtom;
    }
    void setFtypAtom(const HJMovAtom::Ptr& atom) {
        m_ftypAtom = atom;
    }
    HJMovAtom::Ptr& getMoovAtom() {
        return m_moovAtom;
    }
    void setMoovAtom(const HJMovAtom::Ptr& atom) {
        m_moovAtom = atom;
    }
    HJMovAtom::Ptr getMdatAtom();
    
    HJMovAtomList& getAtoms() {
        return m_atoms;
    }
    HJMovAtom::Ptr getAtom(int type)
    {
        for(auto it : m_atoms) {
            if(type == it->getType()) {
                return it;
            }
        }
        return nullptr;
    }
    void addAtom(const HJMovAtom::Ptr& atom);
    
    bool isFastStart() {
        return (!m_moovAtom);
    }
    void setXIO(const HJXIOBase::Ptr& xio) {
        m_xio = xio;
    }
    const HJXIOBase::Ptr& getXIO() {
        return m_xio;
    }
    const HJRange64i getNullRange() {
        return m_nullRange;
    }
public:
    static const std::map<uint32_t, std::string>    ATOM_NAME_MAP;
    static bool validAtom(int type);
    static std::string typeToName(int type);
protected:
    HJMovAtomList       m_atoms;
    HJMovAtom::Ptr      m_ftypAtom = nullptr;
    HJMovAtom::Ptr      m_moovAtom = nullptr;
    bool                m_isFastStart = false;
    HJXIOBase::Ptr     m_xio = nullptr;
    HJRange64i         m_nullRange = HJ_RANGE_ZERO;
};

//***********************************************************************************//
class HJMov2Live : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJMov2Live>;
    HJMov2Live();
    virtual ~HJMov2Live();
    
    int init(const std::string& inUrl);
    int toLive(const std::string& outUrl);
    void done();
public:
    static HJMovAtom::Ptr updateMoovAtom(HJMovAtom::Ptr moovAtom);
protected:
    std::string         m_inUrl = "";
    std::string         m_outUrl = "";
    HJXIOFile::Ptr     m_inFile = nullptr;
    HJXIOFile::Ptr     m_outFile = nullptr;
};

NS_HJ_END
