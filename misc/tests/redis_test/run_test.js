const { MongoMemoryServer } = require('../dependencies/node_modules/mongodb-memory-server');
const { RedisMemoryServer } = require('../dependencies/node_modules/redis-memory-server');

const main = async () => {
    const mongod = await MongoMemoryServer.create();
    const mongodUri = mongod.getUri();

    const redisServer = new RedisMemoryServer();
    const redisHost = await redisServer.getHost();
    const redisPort = await redisServer.getPort();
    
    // console.log('MongoDB URI:', mongodUri);
    // console.log('Redis Host:', redisHost);
    // console.log('Redis Port:', redisPort);

    

    await mongod.stop();
    await redisServer.stop();
};

main().then(() => {
    console.log('done');
    process.exit(0);
}).catch((err) => {
    console.error(err);
    process.exit(1);
});
