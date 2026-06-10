const router = require('express').Router()
const news   = require('../data/news.json')

router.get('/', (req, res) => {
  const base = `${req.protocol}://${req.get('host')}`
  const items = news.map(item => ({
    ...item,
    image: item.image
      ? /^https?:\/\//i.test(item.image) ? item.image : `${base}${item.image}`
      : null,
  }))
  res.json(items)
})

module.exports = router
