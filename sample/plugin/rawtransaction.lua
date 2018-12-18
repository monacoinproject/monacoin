
-- coindから関数を呼び出すために全体を一度実行するので、function外に書かれたコードはOnInit前に実行される

-- rpcの同名コマンドと引数の順番は一緒で、一部の引数は省略可能です
-- アドレスやtxid等、各パラメーターの値は各自の実行環境に合わせて修正してください

------------------------------------------------------
-- gettxoutproof
------------------------------------------------------

print("----------------------------------------------")
print("gettxoutproof:")

txids = {"4bbf9cd3771ff6e8b0f2fdff57254939bcbbd72894253e843dc34e64d1a51350", "ee37de1ebe57db7967a6756023e9eb671db982ee03baa956cf8d25eb7e0f1a61"}

txids_json = cjson.encode(txids)
ret, value = coind.gettxoutproof(txids_json, "e785222479f4ee301458c8367af61f2c85bdc48d5b7470a774dbb3d20b3e9af8")
if ret == true then
  print(string.format("   proof: %s", value))
else
  print("error\n")
end


------------------------------------------------------
-- verifytxoutproof
------------------------------------------------------

print("----------------------------------------------")
print("gettxoutproof:")

proof = "01000030c81f9b12992aa3e983d3459c11b7d854602abae0b74d63939cc242894a78f14c257ebe752c26972b8125e4555280206d2a8ca46137b7ba8936d466235a8469ff0bc6175cffff7f20000000000f00000006a48e55f68c35198fd9aadc5d8c2d337ac8451116cea821e126d1b18a0f98cc2e4c5892963359b623c71f251cd7edc296883314f6c71975363824bcaa942f68a65013a5d1644ec33d843e259428d7bbbc39492557fffdf2b0e8f61f77d39cbf4b611a0f7eeb258dcf56a9ba03ee82b91d67ebe9236075a66779db57be1ede37ee97917cd2b75be659a2cb03a31d833999fa6fcd0320fef45b2603bc64b888d1784f9685a3e49ef861e0cdbdca2cd2028daedfbe6e5ef5829bbdebca1633031c4902dd01"
ret, value = coind.verifytxoutproof(proof)
for i,txid in pairs(value) do
  print(string.format("  txid: %s", txid))
end


------------------------------------------------------
-- getrawtransaction verbose = false
------------------------------------------------------

print("----------------------------------------------")
print("getrawtransaction (verbose = false):")

ret, value = coind.getrawtransaction("5802661f281b8102af844f0b6ebacd695df2264e6937dc4ea2d3983444654332")
if ret == true then
  print(string.format("  data: %s", value))
else
  print("error\n")
end


------------------------------------------------------
-- getrawtransaction verbose = true
------------------------------------------------------

print("----------------------------------------------")
print("getrawtransaction (verbose = true):")

ret, value = coind.getrawtransaction("4bbf9cd3771ff6e8b0f2fdff57254939bcbbd72894253e843dc34e64d1a51350", true)
if ret == true then
  -- 一部だけダンプ
  print(string.format("  hash: %s", value["hash"]))
  print(string.format("  version: %d", value["version"]))
  print(string.format("  locktime: %d", value["locktime"]))
  print(string.format("  vin:"))
  for k, v in pairs( value["vin"] ) do
    print(string.format("    [%d] txid: %s", k, v["txid"]))
    print(string.format("        scriptSig:"))
    print(string.format("          asm: %s", v["scriptSig"]["asm"]))
    print(string.format("          hex: %s", v["scriptSig"]["hex"]))
  end
  print(string.format("  vout:"))
  for k, v in pairs( value["vout"] ) do
    print(string.format("    [%d] value: %f", k, v["value"]))
    print(string.format("        scriptPubKey:"))
    print(string.format("          asm: %s", v["scriptPubKey"]["asm"]))
    print(string.format("          hex: %s", v["scriptPubKey"]["hex"]))
    print(string.format("          address:"))
    for i, addr in pairs( v["scriptPubKey"]["addresses"] ) do
      print(string.format("            [%d] %s", i, addr))
    end
  end
  print(string.format("  confirmations: %d", value["confirmations"]))
  print(string.format("  blocktime: %d", value["blocktime"]))
else
  print("error\n")
end


------------------------------------------------------
-- decoderawtransaction
------------------------------------------------------

print("----------------------------------------------")
print("decoderawtransaction:")

transaction = "0200000001194f54403e8087f429ddaedca3c6ae0c5a8625df8bc13f706b89c513d935db920000000000fdffffff02809698000000000017a9141c5e2db67a1e5be1082012cb5569f78ffa445b7d87809698000000000017a91407d4233f3845e5f28138e410aa702680a61c573a8702000000"
ret, value = coind.decoderawtransaction(transaction)
if ret == true then
  -- 一部だけダンプ
  print(string.format("  hash: %s", value["hash"]))
  print(string.format("  version: %d", value["version"]))
  print(string.format("  locktime: %d", value["locktime"]))
  print(string.format("  vin:"))
  for k, v in pairs( value["vin"] ) do
    print(string.format("    [%d] txid: %s", k, v["txid"]))
    print(string.format("        scriptSig:"))
    print(string.format("          asm: %s", v["scriptSig"]["asm"]))
    print(string.format("          hex: %s", v["scriptSig"]["hex"]))
  end
  print(string.format("  vout:"))
  for k, v in pairs( value["vout"] ) do
    print(string.format("    [%d] value: %f", k, v["value"]))
    print(string.format("        scriptPubKey:"))
    print(string.format("          asm: %s", v["scriptPubKey"]["asm"]))
    print(string.format("          hex: %s", v["scriptPubKey"]["hex"]))
    print(string.format("          address:"))
    for i, addr in pairs( v["scriptPubKey"]["addresses"] ) do
      print(string.format("            [%d] %s", i, addr))
    end
  end
else
  print("error\n")
end


------------------------------------------------------
-- decodescript
------------------------------------------------------

print("----------------------------------------------")
print("decodescript:")

ret, value = coind.decodescript("a914d25b4d27008c7e36601a71eac954295382cb17cf87")
if ret == true then
  print(string.format("  asm: %s", value["asm"]))
  print(string.format("  hex: %s", value["hex"]))
  print(string.format("  type: %s", value["type"]))
  print(string.format("  reqSigs: %d", value["reqSigs"]))
  print(string.format("  address:"))
  for k, v in pairs( value["addresses"] ) do
    print(string.format("    [%d] address: %s", k, v["address"]))
  end
  print(string.format("  p2sh: %s", value["p2sh"]))
else
  print("error\n")
end


------------------------------------------------------
-- createrawtransaction / signrawtransaction / sendrawtransaction
------------------------------------------------------

print("----------------------------------------------")
print("createrawtransaction:")

inputs = {{txid = "9c0beb3fa4ebd918283dcc91dbc7968ab1c96575d2e7f5e0521aacebcfb557d9", vout = 0}}
outputs = {p88ZwicTpUKyiC7JR9gAVm6S81aSQhDT2z = 0.1, p6Fy4dfgLHMuvyLvVLRBdKMobGEuNo9YTc = 0.1}

inputs_json = cjson.encode(inputs)
outputs_json = cjson.encode(outputs)

ret, transaction = coind.createrawtransaction(inputs_json, outputs_json, 0, false)
if ret == true then
  print(string.format("  transaction: %s", transaction))

  print("----------------------------------------------")
  print("signrawtransaction:")

  ret, value = coind.signrawtransaction(transaction)
  if ret == true then
    print(string.format("  hex: %s", value["hex"]))
    print(string.format("  complete: %s", value["complete"] and "true" or "false"))
    for key, val in pairs(value) do
      if key == "errors" then
        print(string.format("  errors:"))
        for k, v in pairs( val ) do
          print(string.format("    [%d] txid: %s", k, v["txid"]))
          print(string.format("    [%d] vout: %d", k, v["vout"]))
          print(string.format("    [%d] scriptSig: %s", k, v["scriptSig"]))
          print(string.format("    [%d] sequence: %d", k, v["sequence"]))
          print(string.format("    [%d] error: %s", k, v["error"]))
        end
      end
    end

    print("----------------------------------------------")
    print("sendrawtransaction:")

    ret, value = coind.sendrawtransaction(value["hex"], true)
    if ret == true then
      print(string.format("  transaction: %s", value))
    end

  end
end


------------------------------------------------------
-- combinerawtransaction
------------------------------------------------------

print("----------------------------------------------")
print("combinerawtransaction:")

txs = {
       "02000000000101fc39ecb72b2b2a8bf100635dde04307b558ef04b7122f7d4e1de1ca0e2e5bad100000000171600149f4cea05e43dea5fc495b4ae1884c448845c5e91fdffffff02809698000000000017a91407d4233f3845e5f28138e410aa702680a61c573a87809698000000000017a9141c5e2db67a1e5be1082012cb5569f78ffa445b7d870247304402205af637b34e1918530922595e762374c0f40e5dd1c05f353f43e3a9ab11b218bc02201489a78e45cea731be5df82092facf0164a94b1b20c1c596bb8fb200d47802bb012103c151e219db69f3eb47e8c21e5206f525357837daecdbdd9f629d2ab26d8a56f100000000",
       "0200000001fd3966a2d7a44e9fd68fef6a7a2f53085edd707ec65a473f0d79940b6880013c000000004847304402201050afad865198df45978b0304eab57d029fba9109efa14dae670fda1fda59d902205ad6d35f1b64997af2058f8fe12f6c6cb610fcc52f13308cf2fa6f52108a144801fdffffff02809698000000000017a9141c5e2db67a1e5be1082012cb5569f78ffa445b7d87809698000000000017a91407d4233f3845e5f28138e410aa702680a61c573a8700000000"
      }

txs_json = cjson.encode(txs)

ret, value = coind.combinerawtransaction(txs_json)
if ret == true then
  print(string.format("  hex: %s", value))
end


