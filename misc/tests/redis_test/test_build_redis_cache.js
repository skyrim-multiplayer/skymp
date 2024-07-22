const { MongoMemoryServer } = require('../dependencies/node_modules/mongodb-memory-server');
const { RedisMemoryServer } = require('../dependencies/node_modules/redis-memory-server/lib');
const { exec } = require('../dependencies/node_modules/@actions/exec/lib/exec');
const { Redis } = require('../dependencies/node_modules/ioredis');
const { MongoClient } = require('../dependencies/node_modules/mongodb');
const path = require('path');
const assert = require('assert');

let mongod, mongodUri, redisServer, redisHost, redisPort, redisClient, mongodbClient;

const main = async () => {
    try {
        mongod = await MongoMemoryServer.create();
        mongodUri = mongod.getUri();
        redisServer = new RedisMemoryServer();
        redisHost = await redisServer.getHost();
        redisPort = await redisServer.getPort();

        redisClient = new Redis({ host: redisHost, port: redisPort });
        mongodbClient = new MongoClient(mongodUri, { useUnifiedTopology: true });

        await mongodbClient.connect();

        const options = {
            cwd: path.normalize("../../../build/dist/server"),
            ignoreReturnCode: true
        };

        const initActorsGamemode = path.normalize("../../../misc/tests/redis_test/test_build_redis_cache.gamemode.init-actors.js")

        const args = [
            `--gamemodePath`,
            initActorsGamemode,
            `--databaseDriver`,
            `mongodb`,
            `--databaseName`,
            `test`,
            `--databaseUri`,
            mongodUri,
            `--databaseRedisCacheUri`,
            `tcp://${redisHost}:${redisPort}`,
        ];
        const res = await exec(`node dist_back/skymp5-server.js`, args, options);

        if (res !== 0) {
            throw new Error('Expected the server to exit with 0, but it exited with ' + res);
        }

        const db = mongodbClient.db('test');
        const changeForms = db.collection('changeForms');
        const version = db.collection('version');

        assert.strictEqual(await changeForms.countDocuments(), 3);
        assert.strictEqual(await version.countDocuments(), 1);

        const found = await version.findOne({});
        assert.strictEqual(found.version, 1);

    } finally {
        await redisClient.quit();
        await mongodbClient.close();
        await mongod.stop();
        await redisServer.stop();
    }
};

main().then(() => {
    console.log('done');
    process.exit(0);
}).catch((err) => {
    console.error(err);
    process.exit(1);
});
