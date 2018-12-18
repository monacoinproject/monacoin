-- httpでjsonを受け取るサンプル
-- https://httpbin.org/json にブラウザでアクセスすれば返ってくる内容を確認できます

local table = require("table")
local string = require("string")
local ltn12 = require("ltn12")
local http = require("socket.http")
local cjson = require("cjson")

local response_body = {}

local res, code, response_headers = http.request{
  url = "https://httpbin.org/json",
  sink = ltn12.sink.table( response_body ),
}

print(res)  -- 関数の返り値
print(code) -- ステータスコード

if code == 200 then
  -- レスポンスヘッダ
  print("Response header:")
  if type(response_headers) == "table" then
    for k, v in pairs(response_headers) do
      print(k, v)
    end
  end

  -- レスポンスボディ
  print("Response body:")
  if type(response_body) == "table" then
    print(table.concat(response_body))
  else
    print("Not a table:", type(response_body))
  end

  -- 返ってきたjsonの解釈
  print("Parse response body:")
  value = cjson.decode(table.concat(response_body))
  print(string.format("title:%s", value["slideshow"]["title"]))
  print(string.format("author:%s", value["slideshow"]["author"]))
  print(string.format("slides:"))
  for i, slide in pairs( value["slideshow"]["slides"] ) do
    for key, val in pairs( slide ) do
      if key == "items" then
        for j, item in pairs( val ) do
          print(string.format("    [%d] [%d] %s", i, j, item))
        end
      else
        print(string.format("  [%d] %s:%s", i, key, val))
      end
    end
  end
end

