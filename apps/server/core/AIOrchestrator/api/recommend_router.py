"""
推荐系统 API
"""
from fastapi import APIRouter, HTTPException
import structlog

from graph.recommendation import RecommendationEngine

logger = structlog.get_logger()
router = APIRouter()


@router.get("/friends")
async def recommend_friends(uid: int, strategy: str = "mixed", limit: int = 10):
    """
    推荐好友

    strategy:
    - connections: 基于共同好友推荐
    - interests: 基于共同兴趣推荐
    - mixed: 混合推荐（默认）
    """
    engine = RecommendationEngine()

    if strategy == "connections":
        results = engine.recommend_friends_common_connections(uid, limit)
    elif strategy == "interests":
        results = engine.recommend_friends_common_interests(uid, limit)
    elif strategy == "2nd_degree":
        results = engine.recommend_friends_2nd_degree(uid, limit)
    elif strategy == "similar":
        results = engine.get_similar_users_by_behavior(uid, limit)
    else:
        results = engine.recommend_friends_mixed(uid, limit)

    return {"code": 0, "strategy": strategy, "recommendations": results}


@router.get("/groups")
async def recommend_groups(uid: int, strategy: str = "interests", limit: int = 10):
    """
    推荐群组

    strategy:
    - interests: 基于共同兴趣话题推荐
    - friends: 基于好友所在的群组推荐
    """
    engine = RecommendationEngine()

    if strategy == "friends":
        results = engine.recommend_groups_via_friends(uid, limit)
    else:
        results = engine.recommend_groups(uid, limit)

    return {"code": 0, "strategy": strategy, "recommendations": results}


@router.get("/topics")
async def recommend_topics(uid: int, since_days: int = 30, limit: int = 20):
    """推荐话题（基于用户历史消息）"""
    engine = RecommendationEngine()
    results = engine.recommend_topics(uid, since_days=since_days, limit=limit)
    return {"code": 0, "recommendations": results}


@router.get("/topics/trending")
async def trending_topics(since_hours: int = 24, limit: int = 20):
    """全站热门话题"""
    engine = RecommendationEngine()
    results = engine.recommend_trending_topics(since_hours=since_hours, limit=limit)
    return {"code": 0, "recommendations": results}
