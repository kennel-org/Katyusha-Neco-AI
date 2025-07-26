# Katyusha Neco AI

ESP-IDFベースのAI猫耳カチューシャプロジェクト。

## 構成
- AtomS3（M5Stack）
- Echo Base（M5Stack）
- Neco（Switch Science、LED70個搭載）
- OpenAI Realtime API連携

## ビルド方法
```sh
idf.py build
idf.py -p <PORT> flash
```

## 機能（初期実装）
- ボタン短押しでAI音声会話起動（stub）

## ハードウェア配線
| 部品 | AtomS3 ピン | 備考 |
|------|-------------|------|
| Echo Base MIC IN | I2S (GPIO1/GPIO2) | on board |
| Neco LED DIN | GPIO38 | 使用LED70個 |
| Button (built-in) | GPIO0 | 起動 / スリープ復帰 |

詳細な回路図は docs/hardware.pdf (後日追加) を参照してください。

## ディレクトリ構成
- main/main.c: メインアプリケーション
- doc/requirements.md: 要件定義

---

今後、各Issueごとに機能追加予定です。
