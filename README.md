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

## ディレクトリ構成
- main/main.c: メインアプリケーション
- doc/requirements.md: 要件定義

---

今後、各Issueごとに機能追加予定です。
