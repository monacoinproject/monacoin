-- postするサンプル
-- OnBlockNotify毎にサーバーに情報をPOSTしてみます

local http = require("socket.http")

function OnBlockNotify(initialsync, hash)
  -- 初期ダウンロードの場合はスルー
  if initialsync == true then
    return
  end

  -- ブロックの情報を取得
  ret, value = coind.getblock(hash)
  if ret ~= true then
    return
  end

  -- HeightとDiffをjsonで送る
  req = {}
  req["Height"] = value["height"]
  req["Difficulty"] = value["difficulty"]
  request_body = cjson.encode(req)
  response_body = {}

  print(string.format("request_body:%s", request_body))

  -- サーバーにPOST
  res, code, response_headers = http.request{
    url = "https://httpbin.org/post",
    method = "POST",
    headers = {
      ["Content-Type"] = "application/x-www-form-urlencoded";
      ["Content-Length"] = #request_body;
    },
    source = ltn12.source.string(request_body),
    sink = ltn12.sink.table(response_body),
  }

  print(res)  -- 関数の返り値
  print(code) -- ステータスコード

  -- レスポンスヘッダ
  print("Response header:")
  if type(response_headers) == "table" then
    for k, v in pairs(response_headers) do
      print(k, v)
    end
  end

  -- https://httpbin.org/post はPOSTしたものをそのまま返してくる
  print(string.format("response_body:%s", request_body))

end

