const router  = require('express').Router()
const modlist = require('../data/modlist.json')

router.get('/', (_req, res) => res.json(modlist))

module.exports = router
