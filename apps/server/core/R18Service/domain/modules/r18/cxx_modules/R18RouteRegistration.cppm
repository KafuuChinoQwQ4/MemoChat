export module memochat.r18.route_registration_algorithms;

export namespace memochat::r18::route_registration::modules
{
const char* GetMethod()
{
    return "GET";
}

const char* PostMethod()
{
    return "POST";
}

const char* SourcesPath()
{
    return "/api/r18/sources";
}

const char* R18AccessStatusPath()
{
    return "/api/r18/access";
}

const char* R18AccessAttestPath()
{
    return "/api/r18/access/attest";
}

const char* SourceImportPath()
{
    return "/api/r18/source/import";
}

const char* SourceEnablePath()
{
    return "/api/r18/source/enable";
}

const char* SourceDisablePath()
{
    return "/api/r18/source/disable";
}

const char* SourceDeletePath()
{
    return "/api/r18/source/delete";
}

const char* SearchPath()
{
    return "/api/r18/search";
}

const char* ComicDetailPath()
{
    return "/api/r18/comic/detail";
}

const char* ChapterPagesPath()
{
    return "/api/r18/chapter/pages";
}

const char* FavoriteTogglePath()
{
    return "/api/r18/favorite/toggle";
}

const char* LibraryPath()
{
    return "/api/r18/library";
}

const char* FavoritesPath()
{
    return "/api/r18/favorites";
}

const char* FolderCreatePath()
{
    return "/api/r18/folder/create";
}

const char* FolderRenamePath()
{
    return "/api/r18/folder/rename";
}

const char* FolderDeletePath()
{
    return "/api/r18/folder/delete";
}

const char* FavoriteAssignPath()
{
    return "/api/r18/favorite/assign";
}

const char* HistoryUpdatePath()
{
    return "/api/r18/history/update";
}

const char* HistoryPath()
{
    return "/api/r18/history";
}

const char* ImagePath()
{
    return "/api/r18/image";
}

const char* AccountsPath()
{
    return "/api/r18/accounts";
}

const char* AccountSavePath()
{
    return "/api/r18/account/save";
}

const char* AccountLoginPath()
{
    return "/api/r18/account/login";
}

const char* AccountClearPath()
{
    return "/api/r18/account/clear";
}

const char* CheckinPath()
{
    return "/api/r18/checkin";
}
} // namespace memochat::r18::route_registration::modules
