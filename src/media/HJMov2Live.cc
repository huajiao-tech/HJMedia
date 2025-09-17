//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJMov2Live.h"

NS_HJ_BEGIN
//***********************************************************************************//
#define HJ_QT_ATOM HJ_BE_FOURCC
/* top level atoms */
#define FREE_ATOM HJ_QT_ATOM('f', 'r', 'e', 'e')
#define JUNK_ATOM HJ_QT_ATOM('j', 'u', 'n', 'k')
#define MDAT_ATOM HJ_QT_ATOM('m', 'd', 'a', 't')
#define MOOV_ATOM HJ_QT_ATOM('m', 'o', 'o', 'v')
#define PNOT_ATOM HJ_QT_ATOM('p', 'n', 'o', 't')
#define SKIP_ATOM HJ_QT_ATOM('s', 'k', 'i', 'p')
#define WIDE_ATOM HJ_QT_ATOM('w', 'i', 'd', 'e')
#define PICT_ATOM HJ_QT_ATOM('P', 'I', 'C', 'T')
#define FTYP_ATOM HJ_QT_ATOM('f', 't', 'y', 'p')
#define UUID_ATOM HJ_QT_ATOM('u', 'u', 'i', 'd')

#define CMOV_ATOM HJ_QT_ATOM('c', 'm', 'o', 'v')
#define STCO_ATOM HJ_QT_ATOM('s', 't', 'c', 'o')
#define CO64_ATOM HJ_QT_ATOM('c', 'o', '6', '4')

#define TRAK_ATOM HJ_QT_ATOM('t', 'r', 'a', 'k')
#define TKHD_ATOM HJ_QT_ATOM('t', 'k', 'h', 'd')
#define MDHD_ATOM HJ_QT_ATOM('m', 'd', 'h', 'd')

#define MDIA_ATOM HJ_QT_ATOM('m', 'd', 'i', 'a')
#define MINF_ATOM HJ_QT_ATOM('m', 'i', 'n', 'f')
#define STBL_ATOM HJ_QT_ATOM('s', 't', 'b', 'l')
#define STSD_ATOM HJ_QT_ATOM('s', 't', 's', 'd')
#define MP4A_ATOM HJ_QT_ATOM('m', 'p', '4', 'a')
#define AVC1_ATOM HJ_QT_ATOM('a', 'v', 'c', '1')

#define HJ_COPY_BUFFER_SIZE        (4 * 1024 * 1024)   //33554432
#define HJ_MAX_FTYP_ATOM_SIZE      (1024 * 1024)

typedef struct {
    uint32_t type;
    uint32_t header_size;
    uint64_t size;
    unsigned char *data;
} atom_t;

typedef struct {
    uint64_t moov_atom_size;
    uint64_t stco_offset_count;
    uint64_t stco_data_size;
    int stco_overflow;
    uint32_t depth;
} update_chunk_offsets_context_t;

typedef struct {
    unsigned char *dest;
    uint64_t original_moov_size;
    uint64_t new_moov_size;
} upgrade_stco_context_t;

typedef int (*parse_atoms_callback_t)(void *context, atom_t *atom);

static int parse_atoms (
    unsigned char *buf,
    uint64_t size,
    parse_atoms_callback_t callback,
    void *context)
{
    unsigned char *pos = buf;
    unsigned char *end = pos + size;
    atom_t atom;
    int ret;

    while (end - pos >= HJ_ATOM_PREAMBLE_SIZE) {
        atom.size = HJ_BE_32(pos);
        atom.type = HJ_BE_32(pos + 4);
        pos += HJ_ATOM_PREAMBLE_SIZE;
        atom.header_size = HJ_ATOM_PREAMBLE_SIZE;

        switch (atom.size) {
        case 1:
            if (end - pos < 8) {
                HJLoge("not enough room for 64 bit atom size");
                return HJErrFatal;
            }

            atom.size = HJ_BE_64(pos);
            pos += 8;
            atom.header_size = HJ_ATOM_PREAMBLE_SIZE + 8;
            break;

        case 0:
            atom.size = HJ_ATOM_PREAMBLE_SIZE + end - pos;
            break;
        }

        if (atom.size < atom.header_size) {
            HJLoge("atom size:" + HJ2STR(atom.size) + "too small");
            return HJErrFatal;
        }

        atom.size -= atom.header_size;

        if (atom.size > end - pos) {
            HJLoge("atom size:" + HJ2STR(atom.size) + "too big");
            return HJErrFatal;
        }

        atom.data = pos;
        ret = callback(context, &atom);
        if (ret < 0) {
            return ret;
        }

        pos += atom.size;
    }

    return HJ_OK;
}

static int update_stco_offsets(update_chunk_offsets_context_t *context, atom_t *atom)
{
    uint32_t current_offset;
    uint32_t offset_count;
    unsigned char *pos;
    unsigned char *end;

    HJLogi(" patching stco atom... ");
    if (atom->size < 8) {
        HJLoge("atom size:" + HJ2STR(atom->size) + "too small");
        return HJErrFatal;
    }

    offset_count = HJ_BE_32(atom->data + 4);
    if (offset_count > (atom->size - 8) / 4) {
        HJLoge("stco offset count:" + HJ2STR(offset_count) + "too big");
        return HJErrFatal;
    }

    context->stco_offset_count += offset_count;
    context->stco_data_size += atom->size - 8;

    for (pos = atom->data + 8, end = pos + offset_count * 4;
        pos < end;
        pos += 4) {
        current_offset = HJ_BE_32(pos);
        if (current_offset > UINT_MAX - context->moov_atom_size) {
            context->stco_overflow = 1;
        }
        current_offset += context->moov_atom_size;
        HJ_WB32(pos, current_offset);
    }

    return HJ_OK;
}

static int update_co64_offsets(update_chunk_offsets_context_t *context, atom_t *atom)
{
    uint64_t current_offset;
    uint32_t offset_count;
    unsigned char *pos;
    unsigned char *end;

    HJLogi(" patching co64 atom... ");
    if (atom->size < 8) {
        HJLoge("atom size:" + HJ2STR(atom->size) + "too small");
        return HJErrFatal;
    }

    offset_count = HJ_BE_32(atom->data + 4);
    if (offset_count > (atom->size - 8) / 8) {
        HJLoge("co64 offset count:" + HJ2STR(offset_count) + "too big");
        return HJErrFatal;
    }

    for (pos = atom->data + 8, end = pos + offset_count * 8;
        pos < end;
        pos += 8) {
        current_offset = HJ_BE_64(pos);
        current_offset += context->moov_atom_size;
        HJ_WB64(pos, current_offset);
    }

    return HJ_OK;
}

static int update_chunk_offsets_callback(void *ctx, atom_t *atom)
{
    update_chunk_offsets_context_t *context = (update_chunk_offsets_context_t *)ctx;
    int ret;

    switch (atom->type) {
    case STCO_ATOM:
        return update_stco_offsets(context, atom);

    case CO64_ATOM:
        return update_co64_offsets(context, atom);

    case MOOV_ATOM:
    case TRAK_ATOM:
    case MDIA_ATOM:
    case MINF_ATOM:
    case STBL_ATOM:
        context->depth++;
        if (context->depth > 10) {
            HJLoge("atoms too deeply nested");
            return HJErrFatal;
        }

        ret = parse_atoms(
            atom->data,
            atom->size,
            update_chunk_offsets_callback,
            context);
        context->depth--;
        return ret;
    }

    return HJ_OK;
}

static void set_atom_size(unsigned char *header, uint32_t header_size, uint64_t size)
{
    switch (header_size) {
    case 8:
        HJ_WB32(header, size);
        break;

    case 16:
        HJ_WB64(header + 8, size);
        break;
    }
}

static void upgrade_stco_atom(upgrade_stco_context_t *context, atom_t *atom)
{
    unsigned char *pos;
    unsigned char *end;
    uint64_t new_offset;
    uint32_t offset_count;
    uint32_t original_offset;

    /* Note: not performing validations since they were performed on the first pass */

    offset_count = HJ_BE_32(atom->data + 4);

    /* write the header */
    memcpy(context->dest, atom->data - atom->header_size, atom->header_size + 8);
    HJ_WB32(context->dest + 4, CO64_ATOM);
    set_atom_size(context->dest, atom->header_size, atom->header_size + 8 + offset_count * 8);
    context->dest += atom->header_size + 8;

    /* write the data */
    for (pos = atom->data + 8, end = pos + offset_count * 4;
        pos < end;
        pos += 4) {
        original_offset = HJ_BE_32(pos) - (uint32_t)context->original_moov_size;
        new_offset = (uint64_t)original_offset + context->new_moov_size;
        HJ_WB64(context->dest, new_offset);
        context->dest += 8;
    }
}

static int upgrade_stco_callback(void *ctx, atom_t *atom)
{
    upgrade_stco_context_t *context = (upgrade_stco_context_t *)ctx;
    unsigned char *start_pos;
    uint64_t copy_size;

    switch (atom->type) {
    case STCO_ATOM:
        upgrade_stco_atom(context, atom);
        break;

    case MOOV_ATOM:
    case TRAK_ATOM:
    case MDIA_ATOM:
    case MINF_ATOM:
    case STBL_ATOM:
        /* write the atom header */
        memcpy(context->dest, atom->data - atom->header_size, atom->header_size);
        start_pos = context->dest;
        context->dest += atom->header_size;

        /* parse internal atoms*/
        if (parse_atoms(
            atom->data,
            atom->size,
            upgrade_stco_callback,
            context) < 0) {
            return -1;
        }

        /* update the atom size */
        set_atom_size(start_pos, atom->header_size, context->dest - start_pos);
        break;

    default:
        copy_size = atom->header_size + atom->size;
        memcpy(context->dest, atom->data - atom->header_size, copy_size);
        context->dest += copy_size;
        break;
    }

    return HJ_OK;
}
//***********************************************************************************//
const std::map<uint32_t, std::string> HJMovFile::ATOM_NAME_MAP = {
    {FREE_ATOM, "free"},
    {JUNK_ATOM, "junk"},
    {MDAT_ATOM, "mdat"},
    {MOOV_ATOM, "moov"},
    {PNOT_ATOM, "pnot"},
    {SKIP_ATOM, "skip"},
    {WIDE_ATOM, "wide"},
    {PICT_ATOM, "PICT"},
    {FTYP_ATOM, "ftyp"},
    {UUID_ATOM, "uuid"},
    
    {CMOV_ATOM, "cmov"},
    {STCO_ATOM, "stco"},
    {CO64_ATOM, "co64"},
    {TRAK_ATOM, "trak"},
    {TKHD_ATOM, "tkhd"},
    {MDHD_ATOM, "mdhd"},
    
    {MDIA_ATOM, "mdia"},
    {MINF_ATOM, "sinf"},
    {STBL_ATOM, "stbl"},
    {STSD_ATOM, "stsd"},
    {MP4A_ATOM, "mp4a"},
    {AVC1_ATOM, "avc1"},
};

int HJMovFile::parse(const HJXIOFile::Ptr& ioFile)
{
    int res = HJ_OK;
    uint32_t atom_type   = 0;
    uint64_t atom_size   = 0;
    uint64_t atom_offset = 0;
    
    HJBuffer::Ptr atomBytes = std::make_shared<HJBuffer>(HJ_ATOM_PREAMBLE_SIZE);
    while (!ioFile->eof())
    {
//        atom_offset = ioFile->tell();
        //
        uint8_t* atomData = atomBytes->data();
        size_t rdcnt = ioFile->read(atomData, HJ_ATOM_PREAMBLE_SIZE);
        if (rdcnt != HJ_ATOM_PREAMBLE_SIZE) {
            res = HJErrIORead;
            HJLoge("read atom size error");
            break;
        }
        atom_size = HJ_BE_32(&atomData[0]);
        atom_type = HJ_BE_32(&atomData[4]);
        
        switch (atom_type) {
            case MOOV_ATOM:
            {
                HJMovAtom::Ptr mdatAtom = getAtom(MDAT_ATOM);
                if (!mdatAtom) {
                    HJLogi("mov is fast start, break");
                    HJMovAtom::Ptr matom = std::make_shared<HJMovAtom>(atom_type, atom_size, atom_offset);
//                    matom->writeData(m_cache->data(), m_cache->size());
                    matom->setAtEnd(false);
                    addAtom(matom);
                    return HJ_OK;
                }
            }
            case FTYP_ATOM:
            {
                HJBuffer::Ptr mdata = std::make_shared<HJBuffer>(atom_size);
                
                res = ioFile->seek(-HJ_ATOM_PREAMBLE_SIZE, std::ios::cur);
                if(HJ_OK != res) {
                    HJLoge("seek error");
                    break;
                }
                rdcnt = ioFile->read(mdata->data(), atom_size);
                if (rdcnt != atom_size) {
                    res = HJErrIORead;
                    HJLoge("read atom data error");
                    break;
                }
                mdata->setSize(rdcnt);
                
                HJMovAtom::Ptr matom = std::make_shared<HJMovAtom>(atom_type, rdcnt, atom_offset);
                matom->setData(mdata);
                addAtom(matom);

                HJLogi("read box type:" + HJ2STR(atom_type) + ", size:" + HJ2STR(rdcnt) + ", offset:" + HJ2STR(atom_offset));
                break;
            }
            default: {
                if (1 == atom_size)
                {
                    size_t rdcnt = ioFile->read(atomData, HJ_ATOM_PREAMBLE_SIZE);
                    if (rdcnt != HJ_ATOM_PREAMBLE_SIZE) {
                        res = HJErrIORead;
                        HJLoge("read atom size error");
                        break;
                    }
                    atom_size = HJ_BE_64(&atomData[0]);
                    res = ioFile->seek(atom_size - 2 * HJ_ATOM_PREAMBLE_SIZE, std::ios::cur);
                } else {
                    res = ioFile->seek(atom_size - HJ_ATOM_PREAMBLE_SIZE, std::ios::cur);
                }
                if(HJ_OK != res) {
                    HJLoge("seek error");
                    break;
                }
                HJMovAtom::Ptr matom = std::make_shared<HJMovAtom>(atom_type, atom_size, atom_offset);
                addAtom(matom);
                //
                if(FREE_ATOM != atom_type) {
                    m_moovAtom = nullptr;
                }
                HJLogi("read box type:" + HJ2STR(atom_type) + ", size:" + HJ2STR(atom_size) + ", offset:" + HJ2STR(atom_offset));
                break;
            }
        }
        atom_offset += atom_size;
        //
        if (atom_size < 8) {
            res = HJErrIORead;
            HJLoge("atom size error");
            break;
        }
    } //while
    HJLogi("parse end");
    
    return res;
}

HJMovAtom::Ptr HJMovFile::getMdatAtom()
{
    return getAtom(MDAT_ATOM);
}

void HJMovFile::addAtom(const HJMovAtom::Ptr& atom)
{
    if(FTYP_ATOM == atom->getType()) {
        m_ftypAtom = atom;
    } else if (MOOV_ATOM == atom->getType()) {
        m_moovAtom = atom;
        if (!m_moovAtom->getAtEnd()) {
            m_nullRange.begin = atom->getOffset();
            m_isFastStart = true;
        }
    } else {
        if (m_atoms.size() <= 0) {
            m_nullRange.begin = atom->getOffset();
            m_nullRange.end = atom->getOffset() + atom->getSize();
        } else {
            m_nullRange.end = atom->getOffset() + atom->getSize();
        }
        m_atoms.push_back(atom);
    }
}

bool HJMovFile::validAtom(int type)
{
    auto it = ATOM_NAME_MAP.find(type);
    if (it != ATOM_NAME_MAP.end()) {
        return true;
    }
    return false;
}

std::string HJMovFile::typeToName(int type)
{
    auto it = ATOM_NAME_MAP.find(type);
    if (it != ATOM_NAME_MAP.end()) {
        return it->second;
    }
    return "";
}

//***********************************************************************************//
HJMov2Live::HJMov2Live()
{
    
}

HJMov2Live::~HJMov2Live()
{
    done();
}

int HJMov2Live::init(const std::string& inUrl)
{
    if (inUrl.empty()) {
        return HJErrInvalidParams;
    }
    m_inUrl = inUrl;
    HJUrl::Ptr murl = std::make_shared<HJUrl>(m_inUrl, HJXFMode_RONLY);
    //
    m_inFile = std::make_shared<HJXIOFile>();
    int res = m_inFile->open(murl);
    if (HJ_OK != res) {
        return res;
    }
    return HJ_OK;
}

int HJMov2Live::toLive(const std::string& outUrl)
{
    if (!m_inFile || outUrl.empty()) {
        return HJErrInvalidParams;
    }
    m_outUrl = outUrl;
    HJLogi("entry, out url:" + outUrl);
    
    int res = HJ_OK;
    do {
        HJMovFile::Ptr movFile = std::make_shared<HJMovFile>();
        res = movFile->parse(m_inFile);
        if (HJ_OK != res) {
            break;
        }
        if (movFile->isFastStart()) {
            HJLogi("url:" + m_inUrl + ", is fast start");
            break;
        }
        HJMovAtom::Ptr moovAtom = movFile->getMoovAtom();
        HJMovAtom::Ptr updateAtom = updateMoovAtom(moovAtom);
        if (!updateAtom) {
            HJLoge("update moov atom error");
            break;
        }
        HJMovAtom::Ptr ftypAtom = movFile->getFtypAtom();
        if (!ftypAtom) {
            HJLoge("update ftyp atom error");
            break;
        }
        m_outFile = std::make_shared<HJXIOFile>();
        res = m_outFile->open(std::make_shared<HJUrl>(m_outUrl, HJXFMode_WONLY));
        if (HJ_OK != res) {
            break;
        }
        m_outFile->write(ftypAtom->getData()->data(), ftypAtom->getSize());
        m_outFile->write(updateAtom->getData()->data(), updateAtom->getSize());
        //
        HJBuffer::Ptr atomBuffer = std::make_shared<HJBuffer>(HJ_COPY_BUFFER_SIZE);
        HJMovAtomList leftAtoms = movFile->getAtoms();
        for (auto atom : leftAtoms)
        {
            size_t bytesTotal = atom->getSize();
            m_inFile->seek(atom->getOffset(), std::ios::beg);
            while (bytesTotal > 0) {
                size_t blockSize = HJ_MIN(bytesTotal, HJ_COPY_BUFFER_SIZE);
                size_t rdsize = m_inFile->read(atomBuffer->data(), blockSize);
                m_outFile->write(atomBuffer->data(), rdsize);
                bytesTotal -= rdsize;
            }
        }
    } while (false);
    HJLogi("end, res:" + HJ2STR(res));
    
    return res;
}

void HJMov2Live::done()
{
    m_inFile = nullptr;
    m_outFile = nullptr;
}

HJMovAtom::Ptr HJMov2Live::updateMoovAtom(HJMovAtom::Ptr moovAtom)
{
    unsigned char* moov_atom = moovAtom->getData()->data();
    uint64_t moov_atom_size = moovAtom->getSize();
    
    update_chunk_offsets_context_t update_context = { 0 };
    upgrade_stco_context_t upgrade_context;
//    unsigned char *new_moov_atom;

    update_context.moov_atom_size = moov_atom_size;

     if (parse_atoms(
         moov_atom,
         moov_atom_size,
         update_chunk_offsets_callback,
         &update_context) < 0) {
         return nullptr;
     }
     if (!update_context.stco_overflow) {
         return moovAtom;
     }

     HJLogi(" upgrading stco atoms to co64... ");
     upgrade_context.new_moov_size = moov_atom_size +
         update_context.stco_offset_count * 8 -
         update_context.stco_data_size;

    HJBuffer::Ptr newBuffer = std::make_shared<HJBuffer>(upgrade_context.new_moov_size);
    if (!newBuffer) {
        HJLoge("could not allocate:" + HJ2STR(upgrade_context.new_moov_size) + " bytes for updated moov atom");
        return nullptr;
    }
    newBuffer->setSize(upgrade_context.new_moov_size);
//     new_moov_atom = (unsigned char *)malloc(upgrade_context.new_moov_size);
//     if (new_moov_atom == NULL) {
//         HJLoge("could not allocate:" + HJ2STR(upgrade_context.new_moov_size) + " bytes for updated moov atom");
//         return HJErrFatal;
//     }

     upgrade_context.original_moov_size = moov_atom_size;
     upgrade_context.dest = newBuffer->data();

     if (parse_atoms(
         moov_atom,
         moov_atom_size,
         upgrade_stco_callback,
         &upgrade_context) < 0)
     {
//         free(new_moov_atom);
         return nullptr;
     }
    if (upgrade_context.dest != newBuffer->data() + newBuffer->size()) {
        HJLoge("unexpected - wrong number of moov bytes written");
        return nullptr;
    }
    HJMovAtom::Ptr newMoovAtom = moovAtom->dup();
    newMoovAtom->setData(newBuffer);

//     free(*moov_atom);
//     *moov_atom = new_moov_atom;
//     *moov_atom_size = upgrade_context.new_moov_size;
//
//     if (upgrade_context.dest != *moov_atom + *moov_atom_size) {
//         fprintf(stderr, "unexpected - wrong number of moov bytes written\n");
//         return -1;
//     }
    return newMoovAtom;
}

NS_HJ_END
