# conncount

netstat | grep xx | wc -l の代替で使えます。

netlink経由で抽出しているので高速軽量です。


## インストール方法

```
$ make
$ cp ./conncount ~/bin/
```

## 使い方

- 80番ポートを利用している全ソケット数を表示
```
$ conncount 80
```

- 80番ポートを利用していてESTABLISHEDなソケット数を表示
```
$ conncount -e 80
```

- 80番ポートを利用しているlisten以外のソケット数を表示（TIME_WAITやFIN_WAIT等を含む）
```
$ conncount -estcf 80
```

```
usage: conncount [OPTION] port
  -l, --listen
  -e, --established
  -t, --time-wait
  -c, --close
  -f, --fin
  -s, --syn
```
