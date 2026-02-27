add_custom_target(VarifyServer
    SOURCES
        config.js
        config.json
        const.js
        email.js
        message.proto
        package-lock.json
        package.json
        proto.js
        redis.js
        server.js
)

set_target_properties(VarifyServer PROPERTIES FOLDER "Server")
