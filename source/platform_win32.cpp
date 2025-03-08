

bool FileExists(String filePath) {
	DWORD dwAttrib = GetFileAttributes(filePath.pData);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool FolderExists(String folderPath) {
	DWORD dwAttrib = GetFileAttributes(folderPath.pData);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
