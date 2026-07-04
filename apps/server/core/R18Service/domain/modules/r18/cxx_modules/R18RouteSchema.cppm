export module memochat.r18.route_schema_algorithms;

export namespace memochat::r18::route_schema::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* SourceEnablePath()
{
    return "/api/r18/source/enable";
}

const char* SourceEnableRouteName()
{
    return "r18.source.enable";
}

const char* SourceDisablePath()
{
    return "/api/r18/source/disable";
}

const char* SourceDisableRouteName()
{
    return "r18.source.disable";
}

const char* SourceToggleRequestTypeName()
{
    return "R18SourceToggleRequestDto";
}

const char* SourceToggleResponseTypeName()
{
    return "R18SourceToggleResponseDto";
}

const char* FavoriteTogglePath()
{
    return "/api/r18/favorite/toggle";
}

const char* FavoriteToggleRouteName()
{
    return "r18.favorite.toggle";
}

const char* FavoriteToggleRequestTypeName()
{
    return "R18FavoriteToggleRequestDto";
}

const char* FavoriteToggleResponseTypeName()
{
    return "R18FavoriteToggleResponseDto";
}

const char* HistoryUpdatePath()
{
    return "/api/r18/history/update";
}

const char* HistoryUpdateRouteName()
{
    return "r18.history.update";
}

const char* HistoryUpdateRequestTypeName()
{
    return "R18HistoryUpdateRequestDto";
}

const char* HistoryUpdateResponseTypeName()
{
    return "R18HistoryUpdateResponseDto";
}
} // namespace memochat::r18::route_schema::modules
