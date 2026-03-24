#include "MongoMgr.h"

MongoMgr::~MongoMgr() {
}

MongoMgr::MongoMgr() {
}

bool MongoMgr::InsertMomentContent(const MomentContentInfo& content) {
    return _dao.InsertMomentContent(content);
}

bool MongoMgr::GetMomentContent(int64_t moment_id, MomentContentInfo& content) {
    return _dao.GetMomentContent(moment_id, content);
}

bool MongoMgr::DeleteMomentContent(int64_t moment_id) {
    return _dao.DeleteMomentContent(moment_id);
}
