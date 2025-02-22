---
sidebar_label: 流式计算
description: "TDengine 流式计算将数据的写入、预处理、复杂分析、实时计算、报警触发等功能融为一体，是一个能够降低用户部署成本、存储成本和运维成本的计算引擎。"
title: 流式计算
---

在时序数据的处理中，经常要对原始数据进行清洗、预处理，再使用时序数据库进行长久的储存。用户通常需要在时序数据库之外再搭建 Kafka、Flink、Spark 等流计算处理引擎，增加了用户的开发成本和维护成本。
使用 TDengine 3.0 的流式计算引擎能够最大限度的减少对这些额外中间件的依赖，真正将数据的写入、预处理、长期存储、复杂分析、实时计算、实时报警触发等功能融为一体，并且，所有这些任务只需要使用 SQL 完成，极大降低了用户的学习成本、使用成本。

## 流式计算的创建

```sql
CREATE STREAM [IF NOT EXISTS] stream_name [stream_options] INTO stb_name AS subquery
stream_options: {
 TRIGGER    [AT_ONCE | WINDOW_CLOSE | MAX_DELAY time]
 WATERMARK   time
 IGNORE EXPIRED
}
```

详细的语法规则参考 [流式计算](../../taos-sql/stream)

## 示例一

企业电表的数据经常都是成百上千亿条的，那么想要将这些分散、凌乱的数据清洗或转换都需要比较长的时间，很难做到高效性和实时性，以下例子中，通过流计算可以将电表电压大于 220V 的数据清洗掉，然后以 5 秒为窗口整合并计算出每个窗口中电流的最大值，最后将结果输出到指定的数据表中。

### 创建 DB 和原始数据表

首先准备数据，完成建库、建一张超级表和多张子表操作

```sql
DROP DATABASE IF EXISTS power;
CREATE DATABASE power;
USE power;

CREATE STABLE meters (ts timestamp, current float, voltage int, phase float) TAGS (location binary(64), groupId int);

CREATE TABLE d1001 USING meters TAGS ("California.SanFrancisco", 2);
CREATE TABLE d1002 USING meters TAGS ("California.SanFrancisco", 3);
CREATE TABLE d1003 USING meters TAGS ("California.LosAngeles", 2);
CREATE TABLE d1004 USING meters TAGS ("California.LosAngeles", 3);
```

### 创建流

```sql
create stream current_stream into current_stream_output_stb as select _wstart as start, _wend as end, max(current) as max_current from meters where voltage <= 220 interval (5s);
```

### 写入数据
```sql
insert into d1001 values("2018-10-03 14:38:05.000", 10.30000, 219, 0.31000);
insert into d1001 values("2018-10-03 14:38:15.000", 12.60000, 218, 0.33000);
insert into d1001 values("2018-10-03 14:38:16.800", 12.30000, 221, 0.31000);
insert into d1002 values("2018-10-03 14:38:16.650", 10.30000, 218, 0.25000);
insert into d1003 values("2018-10-03 14:38:05.500", 11.80000, 221, 0.28000);
insert into d1003 values("2018-10-03 14:38:16.600", 13.40000, 223, 0.29000);
insert into d1004 values("2018-10-03 14:38:05.000", 10.80000, 223, 0.29000);
insert into d1004 values("2018-10-03 14:38:06.500", 11.50000, 221, 0.35000);
```

### 查询以观查结果

```sql
taos> select start, end, max_current from current_stream_output_stb;
          start          |           end           |     max_current      |
===========================================================================
 2018-10-03 14:38:05.000 | 2018-10-03 14:38:10.000 |             10.30000 |
 2018-10-03 14:38:15.000 | 2018-10-03 14:38:20.000 |             12.60000 |
Query OK, 2 rows in database (0.018762s)
```

## 示例二

依然以示例一中的数据为基础，我们已经采集到了每个智能电表的电流和电压数据，现在需要求出有功功率和无功功率，并将地域和电表名以符号 "." 拼接，然后以电表名称分组输出到新的数据表中。

### 创建 DB 和原始数据表

参考示例一 [创建 DB 和原始数据表](#创建-db-和原始数据表)

### 创建流

```sql
create stream power_stream into power_stream_output_stb as select ts, concat_ws(".", location, tbname) as meter_location, current*voltage*cos(phase) as active_power, current*voltage*sin(phase) as reactive_power from meters partition by tbname;
```

### 写入数据

参考示例一 [写入数据](#写入数据)

### 查询以观查结果
```sql
taos> select ts, meter_location, active_power, reactive_power from power_stream_output_stb;
           ts            |         meter_location         |       active_power        |      reactive_power       |
===================================================================================================================
 2018-10-03 14:38:05.000 | California.LosAngeles.d1004    |            2307.834596289 |             688.687331847 |
 2018-10-03 14:38:06.500 | California.LosAngeles.d1004    |            2387.415754896 |             871.474763418 |
 2018-10-03 14:38:05.500 | California.LosAngeles.d1003    |            2506.240411679 |             720.680274962 |
 2018-10-03 14:38:16.600 | California.LosAngeles.d1003    |            2863.424274422 |             854.482390839 |
 2018-10-03 14:38:05.000 | California.SanFrancisco.d1001  |            2148.178871730 |             688.120784090 |
 2018-10-03 14:38:15.000 | California.SanFrancisco.d1001  |            2598.589176205 |             890.081451418 |
 2018-10-03 14:38:16.800 | California.SanFrancisco.d1001  |            2588.728381186 |             829.240910475 |
 2018-10-03 14:38:16.650 | California.SanFrancisco.d1002  |            2175.595991997 |             555.520860397 |
Query OK, 8 rows in database (0.014753s)
```