const appUser = process.env.MEMOCHAT_MONGO_APP_USER;
const appPassword = process.env.MEMOCHAT_MONGO_APP_PASSWORD;

if (!appUser || !appPassword) {
  throw new Error("Mongo application credentials are required");
}

const appDb = db.getSiblingDB("memochat");
if (appDb.getUser(appUser)) {
  appDb.updateUser(appUser, {
    pwd: appPassword,
    roles: [{ role: "readWrite", db: "memochat" }],
  });
} else {
  appDb.createUser({
    user: appUser,
    pwd: appPassword,
    roles: [{ role: "readWrite", db: "memochat" }],
  });
}

for (const collection of ["private_messages", "group_messages", "moments_content"]) {
  if (!appDb.getCollectionNames().includes(collection)) {
    appDb.createCollection(collection);
  }
}
