-- tcpクライアントのサンプル
-- OnBlockNotifyの度にサーバーにメッセージを送信しています

local socket = require("socket")
local tcp

function OnInit()
  -- 接続
  tcp = assert(socket.connect("127.0.0.1", 7500))
end

function OnTerm()
  -- クローズ
  tcp:close()
end

function OnBlockNotify(initialsync, hash)
  -- メッセージ送信
  tcp:send("test message")
end
