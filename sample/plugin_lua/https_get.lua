-- luasecを使ってhttpsでリクエストするサンプル
-- 受け取った内容はstdoutへ書き出し

local string = require("string")
local ltn12 = require("ltn12")
local https = require("ssl.https")

local one, code, headers, status = https.request {
  url = "https://monacoin.org",
  sink = ltn12.sink.file(io.stdout),
  protocol = "any",
  options = "all",
  verify = "none",
}
