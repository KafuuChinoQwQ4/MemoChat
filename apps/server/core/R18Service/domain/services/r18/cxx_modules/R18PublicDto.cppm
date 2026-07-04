export module memochat.r18.public_dto_algorithms;

export namespace memochat::r18::public_dto::modules
{
bool ShouldUseDefaultSourceId(bool source_id_present)
{
    return !source_id_present;
}

int SelectPageOrDefault(bool page_present, int page_value, int default_page)
{
    return page_present ? page_value : default_page;
}

bool SelectFavoriteStateOrDefault(bool favorited_present, bool favorited_value, bool default_favorited)
{
    return favorited_present ? favorited_value : default_favorited;
}

long long SelectPageIndexOrDefault(bool page_index_present, long long page_index_value, long long default_page_index)
{
    return page_index_present ? page_index_value : default_page_index;
}
} // namespace memochat::r18::public_dto::modules
