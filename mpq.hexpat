#include <std/sys.pat>
#include <std/io.pat>
#include <std/mem.pat>

struct MpqBlockEntry {
    u32 file_offset;     // Offset of the file data relative to archive start
    u32 compressed_size; // Size of file inside the archive
    u32 file_size;       // Uncompressed size of the file
    u32 flags;           // Flags (e.g., compressed, encrypted, exists)
};

struct MpqHeaderWotLK {
    char magic[4];                    // "MPQ\x1A"
    u32 header_size;                  // Size of this header (Should be 44 bytes / 0x2C)
    u32 archive_size;                 // Size of the whole archive
    u16 format_version;               // Will display 1 for Version 2 (0-indexed)
    u16 block_size_exponent;          // Sector size = 512 * 2^exponent
    u32 hash_table_offset;            // Offset to the cryptographic hash table
    u32 block_table_offset;           // Offset to the block table
    u32 hash_table_entries;           // Total capacity of hash table
    u32 block_table_entries;          // Total number of files/blocks in archive
    u64 extended_block_table_offset;  // High 16 bits of block offsets
    u16 hash_table_offset_high;       // High 16 bits of hash table offset
    u16 block_table_offset_high;      // High 16 bits of block table offset
};

u64 header_offset = std::mem::find_sequence(0, 0x4D, 0x50, 0x51, 0x1A);

MpqHeaderWotLK header @ header_offset;

u64 high_bits = u64(header.block_table_offset_high);
u64 true_block_table_offset = header.block_table_offset | (high_bits << 32);
u64 absolute_block_table_pos = header_offset + true_block_table_offset;

MpqBlockEntry block_table[header.block_table_entries] @ absolute_block_table_pos;