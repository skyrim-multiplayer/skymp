const express = require("express");
const mongoose = require("mongoose");
const { ApolloServer } = require("apollo-server-express");
const bodyParser = require("body-parser");
const jwt = require("express-jwt");
const cors = require("cors");

const schema = require("./modules");

const config = require("./config");

const server = new ApolloServer({
  schema,
  playground: process.env.NODE_ENV === "development",
  context: (ctx) => ({ user: ctx.req.user }),
});

const app = express();

app.use(cors());
app.use(bodyParser.json());

const authMiddleware = jwt({
  secret: config.jwt.secret,
  credentialsRequired: false,
  algorithms: [config.aes.algorithm],
});

app.use(express.static("dist_ui"));
app.use(authMiddleware);

server.applyMiddleware({ app, path: "/api" });

mongoose
  .connect(
    `mongodb://${config.db.username}:${config.db.password}@${config.db.host}/${config.db.database}`
  )
  .then(() => {
    app.listen(config.port, () => {
      console.log("Server is started on port: ", config.port);
    });
  })
  .catch((err) => console.log("Server start error: ", err));

process.on("exit", (code) => {
  console.log("process exit with code", code);
  mongoose.connection.close();
});
