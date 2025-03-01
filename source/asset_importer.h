
struct String;
struct Arena;

namespace AssetImporter {

int Import(Arena* pScratchArena, u8 format, String source, String output);

};
