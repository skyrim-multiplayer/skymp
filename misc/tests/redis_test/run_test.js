const { MongoMemoryServer } = require('../dependencies/node_modules/mongodb-memory-server');
const { RedisMemoryServer } = require('../dependencies/node_modules/redis-memory-server');
const { exec } = require('../dependencies/node_modules/@actions/exec');
const path = require('path');

const main = async () => {
    const mongod = await MongoMemoryServer.create();
    const mongodUri = mongod.getUri();

    const redisServer = new RedisMemoryServer();
    const redisHost = await redisServer.getHost();
    const redisPort = await redisServer.getPort();

    const options = {
        cwd: path.normalize("../../../build/dist/server"),
        ignoreReturnCode: true
    };

    const testGamemode = path.normalize("../../../misc/tests/redis_test/test_gamemode.js")

    const args = [
        `--gamemodePath`,
        testGamemode,
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

    await mongod.stop();
    await redisServer.stop();

    if (res !== 0) {
        throw new Error('Test failed ' + res);
    }
};

main().then(() => {
    console.log('done');
    process.exit(0);
}).catch((err) => {
    console.error(err);
    process.exit(1);
});
