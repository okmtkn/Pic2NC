# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]
### Added
- QRコードによる自動トリミング機能
- マルチスレッドに対応して処理中にGUIが固まることを防ぐ
- 設定を.iniに保存する
- ファイル出力先が正しいかチェックする
- トリミング時にマウスにルーラーを表示
- 32bit twain driverに対応
- 描画のzoomに対応
- NC dataの編集に対応
- テキストビュワーでアドレスとデータの書式を変更

### Fixed
- Generateボタンを複数回クリックすると何度も処理されてしまう
- Scanner使用時のファイル名を修正

### Changed
- NCデータからトレーリングゼロ消去
- 描画エンジンを変更し線見やすくする


## [2.0.20] - 2023-02-19
### Fixed
- Estimated timeにゼロフィルを追加

## [2.0.19] - 2023-02-17
### Added
- Tablet modeを追加

## [2.0.18] - 2022-09-06
### Changed
- GitへInitial Commit
- ディレクトリ構成を変更
- 他のクラスから操作を簡易化ため，ScannerTwainのpublicメソッドを整理

## [2.0.17] - 2022-09-06
### Changed
- UIを見やすく小変更

## [2.0.16] - 2022-09-04
### Added
- 移動距離，切削距離，加工時間を表示

### Changed
- styleSheetをFusionへ変更

## [2.0.15] - 2022-09-03
### Added
- NC dataのテキストビュワーを追加
- 出力ファイルの拡張子の選択に対応

### Fixed
- Pocket millingのピックフィードが一定に生成されない不具合を修正
- EOBが出力されない不具合を修正

### Changed
- UIをタブインターフェースに変更

## [2.0.14] - 2022-09-01
### Fixed
- トレランスによるcutting pointの削減処理で図形の頂点が選択されないことがある不具合を修正

## [2.0.13] - 2022-08-31
### Added
- Offset Pocket Millingの工具経路出力に対応
- 工具径補正に対応

### Fixed
- 画像選択後に保存先を変更すると，変更が反映されない不具合を修正

### Changed
- cutting pointの削減処理を，トレランスを使った厳密な処理に変更し，NCデータ容量を削減
- 切削点の削減処理を関数化して描画処理のパフォーマンス向上
- cutting codeの設定をメソッドにしてパフォーマンス向上

## [2.0.12] - 2022-08-30
### Added
- 工具経路の画面表示に対応

## [2.0.11] - 2022-08-30
### Changed
- cvCannyの利用をやめて，2値画像をNcDataクラスの入力とし，NcData内部で輪郭を抽出するように変更．
- CuttingPointからis_edge_を廃止
- 距離の計算をさらに簡略化して高速化
- 2-optの計算において，距離コストにリトラクトを加えて，実際の加工時間を短縮
- 孤立したCuttingPointを除去するメソッドを追加

## [2.0.10] - 2022-08-26
### Fixed
- 白色の画像を読み込むとクラッシュする不具合を修正
- 非ループ形状の先端を洗い出してG0する点と設定することで，切削経路の分断を防ぐ

## [2.0.9] - 2022-08-26
### Added
- 2optに対応

## [2.0.8] - 2022-08-23
### Added
- Twainドライバによるスキャナに対応

## [2.0.7] - 2022-07-27
### Added
- アイコンを追加

### Fixed
- 長い処理でGUIが固まって応答がなくなる不具合を解消

### Changed
- push buttonのイベントをreleaseからclickedへ変更

## [2.0.6] - 2022-07-27
### Added
- 手動トリミング機能
- drag & dropによるファイルオープンに対応
- NC Dataをpush buttonでオープンできるように

### Fixed
- 画像以外のファイルをオープンすると画面表示が変わらない不具合を解消

## [2.0.5] - 2022-07-26
### Added
- statusTipを追加
- ウインドウサイズをを正しくリサイズできるようにした( QLabelのsizePolicyをIgnoredに変更)

### Fixed
- ファイルオープンをキャンセルすると.ncdという空の名前が生成される場合があることを修正

### Changed
- 切削経路の最適計算を高速化するために，切削点を注目点の8近傍から探索するように変更
- cv::cannyで生成される不要な点を削除しパフォーマンス向上

## [2.0.4] - 2022-07-24
### Added
- 保存先を指定できるように
- ステータスバーを追加
- About dialogを追加
- NCデータ出力時のビープ音を追加
- ワークサイズを可変にする
- 加工条件を可変にする

### Fixed
- 画像のメモリを二重解放しないよう修正
- 全角のファイル名に対応するため，QImageで読み込んでおいてMatへ変換する

### Changed
- 余計なOpenCVのLibを参照しないようにした

## [2.0.3] - 2022-07-23
### Added
- 切削点のデータ構造を変更し，G0, G1判別のパフォーマンス向上
- 切削点の削減機能を斜め線にも対応し，NCデータの容量を削減
- 2値化をオプションにする

### Changed
- CalcDistanceLite()を整数型に変更
- 画像とワークの縦横両方を考慮してscaleを算出

### Fixed
- File Openの前にスライダを動かした場合に反映されない不具合を解消
- File Openの前にGnererateボタンを押した場合に空のNCデータが生成される不具合を解消
- ポスト処理: モーダルなGコードを省略

## [2.0.2] - 2022-07-23
### Added
- NcDataの初期化を，cv::MatではなくQImageに変更して，opencv.hのインクルードをやめる
- File Open時，拡張子判定を追加

### Fixed
- ファイルオープン時のクラッシュを修正
- 画像処理パラメタ操作時のクラッシュを修正
- File Open時に，画像ファイルかどうか判定してクラッシュしないようにした
- 画像処理パラメタがゼロのときに画像処理をスキップするようにした
- Fileをオープンせずにウインドウサイズをリサイズするとクラッシュした不具合を解消

### Changed
- 画像処理系を別クラスに分割
- cuttig_point_を整数型に変更

## [2.0.1] - 2022-07-22
### Added
- Progress barを実装

## [2.0.0] - 2022-07-17
### Added
- First release
- オープンキャンパスで実使用した
