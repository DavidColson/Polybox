
struct String;
struct Arena;

namespace AssetImporter {

struct ImportTableAsset {
	String outputPath;
	bool enableAutoImport;
	u64 lastImportTime;
	u8 importFormat;
};

struct AssetImportTable {
	Arena* pArena;
	HashMap<String, ImportTableAsset> table;
};

AssetImportTable* LoadImportTable(String project);
int Import(Arena* pScratchArena, u8 format, String source, String output);

};
