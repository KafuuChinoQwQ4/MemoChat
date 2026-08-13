db = db.getSiblingDB("memochat");

const appUser = process.env.MEMOCHAT_MONGO_APP_USER;
const appPassword = process.env.MEMOCHAT_MONGO_APP_PASSWORD;
if (!appUser || !appPassword) {
  throw new Error("MEMOCHAT_MONGO_APP_USER and MEMOCHAT_MONGO_APP_PASSWORD are required");
}

db.createUser({
  user: appUser,
  pwd: appPassword,
  roles: [
    {
      role: "readWrite",
      db: "memochat"
    }
  ]
});

db.createCollection("private_messages");
db.createCollection("group_messages");
