db = db.getSiblingDB("memochat");

db.createUser({
  user: "memochat_app",
  pwd: "123456",
  roles: [
    {
      role: "readWrite",
      db: "memochat"
    }
  ]
});

db.createCollection("private_messages");
db.createCollection("group_messages");
