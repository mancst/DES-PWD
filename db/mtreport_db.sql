-- MySQL dump 10.13  Distrib 5.6.50, for Linux (x86_64)
--
-- Host: localhost    Database: mtreport_db
-- ------------------------------------------------------
-- Server version	5.6.50

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES latin1 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Current Database: `mtreport_db`
--

/*!40000 DROP DATABASE IF EXISTS `mtreport_db`*/;

CREATE DATABASE /*!32312 IF NOT EXISTS*/ `mtreport_db` /*!40100 DEFAULT CHARACTER SET latin1 */;

USE `mtreport_db`;

--
-- Table structure for table `flogin_history`
--

DROP TABLE IF EXISTS `flogin_history`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `flogin_history` (
  `xrk_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `user_id` int(11) unsigned NOT NULL,
  `login_time` int(12) unsigned DEFAULT '0',
  `login_remote_address` char(16) DEFAULT '0.0.0.0',
  `receive_server` char(16) DEFAULT '0.0.0.0',
  `referer` varchar(1024) DEFAULT NULL,
  `user_agent` varchar(1024) DEFAULT NULL,
  `method` int(10) DEFAULT '0',
  `valid_time` int(11) unsigned DEFAULT NULL,
  PRIMARY KEY (`xrk_id`),
  KEY `user_id` (`user_id`)
) ENGINE=MyISAM AUTO_INCREMENT=327 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `flogin_history`
--

LOCK TABLES `flogin_history` WRITE;
/*!40000 ALTER TABLE `flogin_history` DISABLE KEYS */;
INSERT INTO `flogin_history` VALUES (324,1,1620454958,'192.168.128.1','192.168.128.128','http://192.168.128.122/cgi-bin/slog_flogin?action=show_main&login_show=var_css','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.93 Safari/537.36',0,1621059758),(325,1,1620455854,'192.168.128.1','192.168.128.128','http://192.168.128.122/cgi-bin/slog_flogin?action=show_main&login_show=var_css','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.93 Safari/537.36',0,1621060654),(326,1,1620456266,'192.168.128.1','192.168.128.128','http://192.168.128.122/cgi-bin/slog_flogin?action=show_main&login_show=var_css','Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.93 Safari/537.36',0,1621061066);
/*!40000 ALTER TABLE `flogin_history` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `flogin_user`
--

DROP TABLE IF EXISTS `flogin_user`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `flogin_user` (
  `user_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `user_name` varchar(64) NOT NULL,
  `user_pass_md5` varchar(128) NOT NULL,
  `login_type` int(11) NOT NULL DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `rmark` varchar(128) DEFAULT NULL,
  `dwReserved1` int(11) unsigned DEFAULT '0',
  `dwReserved2` int(11) unsigned DEFAULT '0',
  `strReserved1` varchar(64) DEFAULT NULL,
  `strReserved2` varchar(64) DEFAULT NULL,
  `user_flag_1` int(12) unsigned DEFAULT '0',
  `last_login_time` int(12) unsigned DEFAULT '0',
  `register_time` int(12) unsigned DEFAULT '0',
  `last_login_address` char(16) DEFAULT '0.0.0.0',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `xrk_status` tinyint(4) DEFAULT '0',
  `email` varchar(64) DEFAULT NULL,
  `login_index` int(11) DEFAULT '0',
  `login_md5` varchar(32) DEFAULT '',
  `last_login_server` char(16) DEFAULT '0.0.0.0',
  `bind_DES-PWE_uid` int(11) unsigned DEFAULT '0',
  `other_info` blob,
  `bind_DES-PWE_key` varchar(32) DEFAULT '',
  `other_info_seq` int(11) unsigned DEFAULT '0',
  PRIMARY KEY (`user_id`),
  KEY `idx_name` (`user_name`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `flogin_user`
--

LOCK TABLES `flogin_user` WRITE;
/*!40000 ALTER TABLE `flogin_user` DISABLE KEYS */;
INSERT INTO `flogin_user` VALUES (1,'sadmin','c5edac1b8c1d58bad90a246d8f08f53b',1,'2021-05-08 06:46:27','supperuser',1552345835,152,NULL,'0c96aa43ce31757758c40be976c20d34',1,1620456266,0,'192.168.128.1',1,1,0,'4033@qq.com',0,'c5edac1b8c1d58bad90a246d8f08f53b','192.168.128.128',0,'v3.4','',197541);
/*!40000 ALTER TABLE `flogin_user` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger flogin_user_up after update on flogin_user for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('flogin_user', old.user_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_app_info`
--

DROP TABLE IF EXISTS `mt_app_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_app_info` (
  `app_name` varchar(32) NOT NULL,
  `app_desc` varchar(128) NOT NULL,
  `app_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `create_time` int(12) unsigned DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `user_add` varchar(64) NOT NULL,
  `user_mod` varchar(64) NOT NULL,
  `xrk_status` tinyint(4) DEFAULT '0',
  `app_type` int(10) unsigned DEFAULT '2',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  PRIMARY KEY (`app_id`)
) ENGINE=InnoDB AUTO_INCREMENT=120 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_app_info`
--

LOCK TABLES `mt_app_info` WRITE;
/*!40000 ALTER TABLE `mt_app_info` DISABLE KEYS */;
INSERT INTO `mt_app_info` VALUES ('迁移云管系统','迁移云管系统',30,0,'2019-01-25 23:42:34','sadmin','sadmin',0,3,1,1),('监控插件','全部插件归属到该应用下，每个插件以插件名在该应用下创建模块',119,1584529976,'2020-03-18 11:12:56','sadmin','sadmin',0,2,1,1);
/*!40000 ALTER TABLE `mt_app_info` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_app_info_ins after insert on mt_app_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_app_info', new.app_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_app_info_up after update on mt_app_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_app_info', old.app_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_attr`
--

DROP TABLE IF EXISTS `mt_attr`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_attr` (
  `xrk_id` int(11) NOT NULL AUTO_INCREMENT,
  `attr_type` int(8) DEFAULT '0' COMMENT 'å±žæ€§ç±»åž‹',
  `data_type` int(6) DEFAULT '0' COMMENT 'æ•°æ®ç±»åž‹',
  `user_add` varchar(64) DEFAULT 'sadmin' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `attr_name` varchar(64) NOT NULL COMMENT 'å±žæ€§åç§°',
  `attr_desc` varchar(256) DEFAULT NULL COMMENT 'å±žæ€§æè¿°',
  `xrk_status` tinyint(4) DEFAULT '0',
  `user_add_id` int(11) unsigned DEFAULT '0',
  `user_mod_id` int(11) unsigned DEFAULT '0',
  `excep_attr_mask` tinyint(4) DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `static_time` int(11) unsigned DEFAULT '1',
  `value_type` int(11) unsigned DEFAULT '0',
  `chart_type` int(11) unsigned DEFAULT '0',
  PRIMARY KEY (`xrk_id`)
) ENGINE=InnoDB AUTO_INCREMENT=358 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_attr`
--

LOCK TABLES `mt_attr` WRITE;
/*!40000 ALTER TABLE `mt_attr` DISABLE KEYS */;
INSERT INTO `mt_attr` VALUES (62,51,1,'sadmin','模块日志记录发送量','模块日志记录发送量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(64,51,1,'sadmin','收到失败的日志响应包量123','收到失败的日志响应包量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(65,51,1,'sadmin','收到日志响应包量','收到日志响应包量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(67,55,1,'sadmin','云平台整体负载监测','写入日志记录量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(70,51,1,'sadmin','日志发送包量','日志发送包量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(73,53,1,'sadmin','pb 格式日志包量','pb 格式日志包量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(74,53,3,'sadmin','收到非法日志包量','收到非法日志包量',0,1,1,1,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(75,53,1,'sadmin','收到的日志记录量','收到的日志记录量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(84,55,1,'sadmin','输出日志文件占用磁盘空间到文件','输出日志文件占用磁盘空间到文件',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(93,51,1,'sadmin','slog_client 心跳上报量','日志客户端心跳上报',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(94,53,1,'sadmin','收到Log心跳上报量','来自 slog_client 的心跳',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(100,67,1,'sadmin','数据存入vmem量','数据存入vmem量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(101,67,3,'sadmin','数据存入vmem失败量','数据存入vmem失败量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(102,67,1,'sadmin','从vmem 取出数据成功量','从vmem 取出数据成功量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(103,67,3,'sadmin','从vmem 取出数据异常','从vmem 取出数据异常',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(104,67,3,'sadmin','vmem 释放异常','vmem 释放异常',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(105,67,1,'sadmin','vmem 延迟释放量','vmem 延迟释放量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(106,67,1,'sadmin','vmem 延迟释放成功','vmem 延迟释放',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(108,69,3,'sadmin','cgi 启动失败','cgi 执行失败',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(110,69,1,'sadmin','login cookie check 失败','login cookie check 失败',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(111,70,1,'sadmin','login cookie check 成功量123','login cookie check 成功量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(112,70,1,'sadmin','login 弹出登录框','login 弹出登录框',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(113,70,1,'sadmin','login 通过输入用户名密码登录成功','login 通过输入用户名密码登录成功',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(116,71,3,'sadmin','mysql 连接失败','mysql 连接失败',0,1,1,1,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(117,71,1,'sadmin','mysql 连接耗时0-10ms','mysql 连接耗时0-10ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(118,71,1,'sadmin','mysql 连接耗时10-20ms','mysql 连接耗时10-20ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(119,71,1,'sadmin','mysql 连接耗时20-50ms','mysql 连接耗时20-50ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(120,71,1,'sadmin','mysql 连接耗时50-100ms','mysql 连接耗时50-100ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(121,71,1,'sadmin','mysql 连接耗时大于1000ms','mysql 连接耗时大于1000ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(122,71,1,'sadmin','mysql 执行时间 0-20 ms','mysql 执行时间 0-20 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(123,71,1,'sadmin','mysql 执行时间 20-50 ms','mysql 执行时间 20-50 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(124,71,1,'sadmin','mysql 执行时间 50-100 ms','mysql 执行时间 50-100 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(125,71,1,'sadmin','mysql 执行时间 100-200 ms','mysql 执行时间 100-200 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(126,71,1,'sadmin','mysql 执行时间 200-500 ms','mysql 执行时间 200-500 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(127,71,1,'sadmin','mysql 执行时间 500-1000 ms','mysql 执行时间 500-1000 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(128,71,1,'sadmin','mysql 执行时间 1000-2000 ms','mysql 执行时间 1000-2000 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(129,71,1,'sadmin','mysql 执行时间大于 2000','mysql 执行时间大于 2000 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(130,71,1,'sadmin','mysql 连接耗时在 100-200 ms','mysql 连接耗时在 100-200 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(131,71,1,'sadmin','mysql 连接耗时在 200-500 ms','mysql 连接耗时在 200-500 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(132,71,1,'sadmin','mysql 连接耗时在 500-1000 ms','mysql 连接耗时在 500-1000 ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(133,71,1,'sadmin','mysql get result 完成时间 0-20ms','mysql get result 完成时间 0-20ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(134,71,1,'sadmin','mysql get result 完成时间 20-50ms','mysql get result 完成时间 20-50ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(135,71,1,'sadmin','mysql get result 完成时间 50-100ms','mysql get result 完成时间 50-100ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(136,71,1,'sadmin','mysql get result 完成时间 100-200ms','mysql get result 完成时间 100-200ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(137,71,1,'sadmin','mysql get result 完成时间 200-500ms','mysql get result 完成时间 200-500ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(138,71,1,'sadmin','mysql get result 完成时间 500-1000ms','mysql get result 完成时间 500-1000ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(139,71,1,'sadmin','mysql get result 完成时间大于1000ms','mysql get result 完成时间大于1000ms',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(142,69,1,'sadmin','cgi 请求失败量','cgi 请求失败量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(143,70,1,'sadmin','存储服务响应成功量','cgi 请求成功量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(144,72,1,'sadmin','cgi 响应耗时在 0-100ms123 ','cgi 响应耗时在 0-100ms ',0,1,15,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(145,72,1,'sadmin','cgi 响应耗时在 100-300 ms','cgi 响应耗时在 100-300 ms',0,15,15,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(146,72,1,'sadmin','cgi 响应耗时在 300-500 ms','cgi 响应耗时在 300-500 ms',0,15,15,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(147,72,1,'sadmin','cgi 响应耗时大于等于 500 ms','cgi 响应耗时大于等于 500 ms',0,15,15,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(203,80,1,'sadmin','memcache get 成功量','memcache get 成功量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(204,80,1,'sadmin','memcache get 总量','memcache get 总量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(205,80,1,'sadmin','memcache get 失败量','memcache get 失败量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(206,80,1,'sadmin','memcache set 失败量','memcache set 失败量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(207,80,1,'sadmin','memcache set 成功量','memcache set 成功量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(208,80,1,'sadmin','应用程序响应时间监测','memcache set 总量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(220,53,1,'sadmin','client log上报时间触发校准','client 与 server 的时间相差超过2分钟',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(223,82,1,'sadmin','邮件过期未发送量','邮件过期未发送量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(224,82,1,'sadmin','邮件发送量','总邮件发送量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(225,82,1,'sadmin','邮件发送失败量','总邮件发送失败',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(234,55,3,'sadmin','从vmem 获取日志内容出错','从vmem 获取日志内容出错',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(235,55,3,'sadmin','校验vmem 日志出错','校验vmem 日志出错',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(254,53,1,'sadmin','删除app 日志文件请求量','删除app 日志文件请求量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(255,53,1,'sadmin','查询 app 日志文件占用空间请求量','查询 app 日志文件占用空间请求量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(256,53,1,'sadmin','反序列化请求包失败','反序列化请求包失败',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(257,53,1,'sadmin','收到查询 app 日志占用空间请求量','收到查询 app 日志占用空间请求量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(258,53,1,'sadmin','收到查询 app 日志占用空间响应量','收到查询 app 日志占用空间响应量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(259,53,1,'sadmin','发送查询 app 日志占用空间响应量','发送查询 app 日志占用空间响应量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(261,83,3,'sadmin','测试云版本提示通道','用于测试云版本提示通道',0,1,1,0,'2020-04-08 08:43:58','2020-04-08 08:35:16',1,0,0),(262,53,1,'sadmin','设置日志文件删除标记量','设置日志文件删除标记量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(263,55,1,'sadmin','通过删除标记删除日志文件量','通过删除标记删除日志文件量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(284,83,1,'sadmin','当前提示产生量','当前提示产生量',0,1,1,0,'2019-09-04 07:48:55','0000-00-00 00:00:00',1,0,0),(285,55,1,'sadmin','日志写入磁盘量（单位byte）','日志写入磁盘量  (单位byte）',0,1,1,0,'2019-09-04 07:46:09','0000-00-00 00:00:00',1,0,0),(330,53,5,'sadmin','远程总日志记录量','agent client 发送过来的日志量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(331,53,5,'sadmin','迁移云管系统本身的日志记录量','迁移云管系统本身的日志记录量',0,1,1,0,'2019-08-31 11:26:58','0000-00-00 00:00:00',1,0,0),(332,55,5,'sadmin','存储服务组件调用总量','日志总量',0,1,1,0,'2019-06-06 06:14:36','0000-00-00 00:00:00',1,0,0),(355,82,1,'sadmin','邮件发送量','每分钟的邮件发送量',0,1,1,0,'2019-07-03 08:27:27','0000-00-00 00:00:00',1,0,0),(356,82,5,'sadmin','邮件发送量历史总量','总的邮件发送量',0,1,1,0,'2019-07-03 08:27:53','0000-00-00 00:00:00',1,0,0),(357,83,3,'sadmin','测试云框架提示通道','用于测试云框架提示通道',0,1,1,0,'2020-11-09 12:57:39','2020-11-09 12:57:39',1,0,0);
/*!40000 ALTER TABLE `mt_attr` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_ins after insert on mt_attr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_attr', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_up after update on mt_attr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_attr', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_attr_type`
--

DROP TABLE IF EXISTS `mt_attr_type`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_attr_type` (
  `xrk_type` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `parent_type` int(11) unsigned DEFAULT '0',
  `type_pos` varchar(128) DEFAULT '0' COMMENT 'ç±»åž‹ä½ç½®',
  `xrk_name` varchar(64) NOT NULL,
  `attr_desc` varchar(256) DEFAULT NULL COMMENT 'ç±»åž‹æè¿°',
  `xrk_status` tinyint(4) DEFAULT '0',
  `create_user` varchar(64) DEFAULT NULL,
  `mod_user` varchar(64) DEFAULT NULL,
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `user_add_id` int(11) unsigned DEFAULT '0',
  `user_mod_id` int(11) unsigned DEFAULT '0',
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`xrk_type`)
) ENGINE=InnoDB AUTO_INCREMENT=85 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_attr_type`
--

LOCK TABLES `mt_attr_type` WRITE;
/*!40000 ALTER TABLE `mt_attr_type` DISABLE KEYS */;
INSERT INTO `mt_attr_type` VALUES (1,0,'1','监控点根类型','监控点根类型',0,'sadmin','sadmin','2019-07-02 06:39:16',1,1,'0000-00-00 00:00:00'),(48,1,'1.1','迁移云管系统','迁移云管系统自身的相关监控',0,'sadmin','sadmin','2020-03-18 14:03:13',1,1,'2020-03-18 13:26:01'),(50,48,'1.1.1','日志相关','日志监控相关模块的监控',0,'sadmin','sadmin','2020-03-18 14:02:55',1,1,'0000-00-00 00:00:00'),(51,50,'1.1.1.1','slog_client','日志系统客户端相关监控上报',0,'sadmin','sadmin','2020-03-18 13:54:43',1,1,'0000-00-00 00:00:00'),(53,50,'1.1.1.1','slog_serve','slog 日志服务器端',0,'sadmin','sadmin','2020-03-18 13:54:43',1,1,'0000-00-00 00:00:00'),(54,50,'1.1.1.1','slog_config','读取 mysql 配置的进程',0,'sadmin','sadmin','2020-03-18 13:54:43',1,1,'0000-00-00 00:00:00'),(55,50,'1.1.1.1','slog_write','日志服务器端写日志进程的相关上报',0,'sadmin','sadmin','2020-03-18 13:54:43',1,1,'0000-00-00 00:00:00'),(56,48,'1.1.1','监控点相关','监控点相关模块',0,'sadmin','sadmin','2020-03-18 14:02:55',1,1,'0000-00-00 00:00:00'),(57,56,'1.1.1.1','monitor_client','迁移云管系统客户端 monitor_client',0,'sadmin','sadmin','2020-03-18 13:54:57',1,1,'0000-00-00 00:00:00'),(58,56,'1.1.1.1','monitor_server','迁移云管系统服务器端 slog_monitor_server',0,'sadmin','sadmin','2020-03-18 13:54:57',1,1,'0000-00-00 00:00:00'),(66,48,'1.1.1','通用组件上报','通用组件监控上报',0,'sadmin','sadmin','2020-03-18 14:02:55',1,1,'0000-00-00 00:00:00'),(67,66,'1.1.1.1','vmem','vmem 可变长共享内存组件',0,'sadmin','sadmin','2020-03-18 13:55:02',1,1,'0000-00-00 00:00:00'),(68,48,'1.1.1','cgi 监控','监控点系统 cgi/fcgi 相关的监控上报',0,'sadmin','sadmin','2020-03-18 14:02:55',1,1,'0000-00-00 00:00:00'),(69,68,'1.1.1.1','cgi 异常监控','cgi 异常点上报s',0,'sadmin','sadmin','2020-03-18 13:55:04',1,15,'0000-00-00 00:00:00'),(70,68,'1.1.1.1','cgi 业务量监控','cgi 业务量监控',0,'sadmin','sadmin','2020-03-18 13:55:04',1,1,'0000-00-00 00:00:00'),(71,66,'1.1.1.1','mysqlwrappe','mysql 组件',0,'sadmin','sadmin','2020-03-18 13:55:02',1,1,'0000-00-00 00:00:00'),(72,68,'1.1.1.1','cgi 调用耗时监控','迁移云管系统 cgi 调用耗时监控',0,'sadmin','sadmin','2020-03-18 13:55:04',1,1,'0000-00-00 00:00:00'),(80,66,'1.1.1.1','memcache','memcache 监控',0,'sadmin','sadmin','2020-03-18 13:55:02',1,1,'0000-00-00 00:00:00'),(82,66,'1.1.1.1','mail','邮件发送监控',0,'sadmin','sadmin','2020-03-18 13:55:02',1,1,'0000-00-00 00:00:00'),(83,56,'1.1.1.1','迁移提示相关','',0,'sadmin','sadmin','2020-03-18 13:54:57',1,1,'0000-00-00 00:00:00'),(84,1,'1.1','监控插件','所有监控插件监控点类型的根节点',0,'sadmin','sadmin','2020-03-18 11:07:32',1,1,'2020-03-18 11:07:32');
/*!40000 ALTER TABLE `mt_attr_type` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_type_ins after insert on mt_attr_type for each row begin insert into mt_table_upate_monitor
(u_table_name, r_primary_id) values('mt_attr_type', new.xrk_type); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_attr_type_up after update on mt_attr_type for each row begin insert into mt_table_upate_monitor(
u_table_name, r_primary_id) values('mt_attr_type', old.xrk_type); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_log_config`
--

DROP TABLE IF EXISTS `mt_log_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_log_config` (
  `app_id` int(11) unsigned DEFAULT '0',
  `module_id` int(11) unsigned DEFAULT '0',
  `log_types` int(10) unsigned DEFAULT '0',
  `config_name` varchar(32) NOT NULL,
  `config_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `create_time` int(12) unsigned DEFAULT '0',
  `user_add` varchar(64) NOT NULL,
  `user_mod` varchar(64) NOT NULL,
  `xrk_status` tinyint(4) DEFAULT '0',
  `config_desc` varchar(128) DEFAULT NULL,
  `user_add_id` int(11) unsigned DEFAULT '1',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `write_speed` int(10) unsigned DEFAULT '0',
  PRIMARY KEY (`config_id`)
) ENGINE=InnoDB AUTO_INCREMENT=179 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_log_config`
--

LOCK TABLES `mt_log_config` WRITE;
/*!40000 ALTER TABLE `mt_log_config` DISABLE KEYS */;
INSERT INTO `mt_log_config` VALUES (30,26,127,'fcgi_mt_slog',34,'2019-08-26 12:47:52',0,'sadmin','sadmin',0,'fcgi_mt_slog',1,1,0),(30,25,127,'fcgi_slog_flogin',35,'2019-08-26 12:47:38',0,'sadmin','sadmin',0,'fcgi_slog_flogin',1,1,0),(30,24,127,'slog_client',36,'2019-08-26 12:47:25',0,'sadmin','sadmin',0,'slog_client',1,1,0),(30,23,127,'slog_write',37,'2019-08-26 12:47:15',0,'sadmin','sadmin',0,'slog_write',1,1,0),(30,22,127,'slog_server',38,'2019-08-26 12:47:03',0,'sadmin','sadmin',0,'slog_server',1,1,0),(30,21,127,'slog_config',39,'2019-08-26 12:46:54',0,'sadmin','sadmin',0,'slog_config',1,1,0),(30,27,127,'slog_mtreport_server',40,'2019-08-26 12:48:09',0,'sadmin','sadmin',0,'slog_mtreport_server',1,1,0),(30,28,127,'fcgi_mt_slog_monitor',41,'2019-08-26 12:46:43',0,'sadmin','sadmin',0,'cgi_monitor',1,1,0),(30,29,127,'fcgi_mt_slog_attr',42,'2019-08-26 12:46:29',0,'sadmin','sadmin',0,'mt_slog_attr',1,1,0),(30,30,127,'fcgi_mt_slog_machine',43,'2019-08-26 12:48:29',0,'sadmin','sadmin',0,'fcgi_mt_slog_machine',1,1,0),(30,31,127,'fcgi_mt_slog_view',44,'2019-08-26 12:49:26',0,'sadmin','sadmin',0,'fcgi_mt_slog_view',1,1,0),(30,33,127,'slog_monitor_server',46,'2019-08-26 12:45:58',0,'sadmin','sadmin',0,'config_monitor_server',1,1,0),(30,34,127,'fcgi_mt_slog_showview',47,'2019-08-26 12:49:43',0,'sadmin','sadmin',0,'fcgi_mt_slog_showview',1,1,0),(30,35,127,'fcgi_mt_slog_warn',48,'2019-08-31 11:06:51',0,'sadmin','sadmin',0,'fcgi_mt_slog_warn',1,1,0),(30,39,127,'fcgi_mt_slog_user',52,'2019-08-31 11:06:59',0,'sadmin','sadmin',0,'fcgi_mt_slog_user',1,1,222),(30,47,255,'slog_monitor_client',62,'2019-05-24 02:43:17',1501204510,'sadmin','sadmin',0,'server attr client',1,1,0),(30,33,127,'slog_monitor_server',63,'2019-08-27 00:14:59',1501219043,'sadmin','sadmin',0,' attr server',1,1,4200),(30,48,127,'slog_deal_warn',64,'2019-08-26 12:45:20',1502026252,'sadmin','sadmin',0,'mail',1,1,0),(30,59,127,'slog_check_warn',74,'2019-08-26 12:45:27',1533356355,'sadmin','sadmin',0,'提示模块',1,1,0),(30,150,108,'fcgi_mt_slog_reportinfo',178,'2020-05-04 22:19:28',1588630768,'sadmin','sadmin',0,'公共上报cgi 日志配置',1,1,0);
/*!40000 ALTER TABLE `mt_log_config` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_log_config_ins after insert on mt_log_config for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_log_config', new.config_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_log_config_up after update on mt_log_config for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_log_config', old.config_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_machine`
--

DROP TABLE IF EXISTS `mt_machine`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_machine` (
  `xrk_id` int(11) NOT NULL AUTO_INCREMENT,
  `xrk_name` varchar(64) NOT NULL,
  `ip1` int(12) unsigned DEFAULT NULL,
  `ip2` int(12) unsigned DEFAULT NULL,
  `ip3` int(12) unsigned DEFAULT NULL,
  `ip4` int(12) unsigned DEFAULT NULL,
  `user_add` varchar(64) DEFAULT 'sadmin' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `user_mod` varchar(64) DEFAULT 'sadmin' COMMENT 'æœ€åŽæ›´æ–°ç”¨æˆ·',
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'æ›´æ–°æ—¶é—´',
  `machine_desc` varchar(256) DEFAULT NULL COMMENT 'æœºå™¨æè¿°',
  `xrk_status` tinyint(4) DEFAULT '0',
  `warn_flag` int(8) DEFAULT '0' COMMENT 'å‘Šè­¦æ ‡è®°',
  `model_id` int(8) DEFAULT '0' COMMENT 'æœºå™¨åž‹å·',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `last_attr_time` int(12) unsigned DEFAULT '0',
  `last_log_time` int(12) unsigned DEFAULT '0',
  `last_hello_time` int(12) unsigned DEFAULT '0',
  `start_time` int(12) unsigned DEFAULT '0',
  `cmp_time` varchar(32) DEFAULT NULL,
  `agent_version` varchar(20) DEFAULT NULL,
  `agent_os` varchar(32) DEFAULT NULL,
  `libc_ver` varchar(64) DEFAULT NULL,
  `libcpp_ver` varchar(64) DEFAULT NULL,
  `os_arc` varchar(64) DEFAULT NULL,
  `rand_key` char(18) DEFAULT NULL,
  `gw_ip` int(12) unsigned DEFAULT NULL,
  `conn_ip` int(12) unsigned DEFAULT NULL,
  `cfg_ttl` int(11) unsigned DEFAULT NULL,
  `recv_ttl` int(11) unsigned DEFAULT NULL,
  `client_ip` int(12) unsigned DEFAULT NULL,
  `server_response_time_ms` int(11) unsigned DEFAULT NULL,
  PRIMARY KEY (`xrk_id`),
  KEY `ip1` (`ip1`)
) ENGINE=InnoDB AUTO_INCREMENT=134 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_machine`
--

LOCK TABLES `mt_machine` WRITE;
/*!40000 ALTER TABLE `mt_machine` DISABLE KEYS */;
INSERT INTO `mt_machine` VALUES (124,'comm_report',16777343,0,0,0,'sadmin','sadmin','2020-05-05 02:32:53','用于接收公共上报的虚拟机器',0,1,2,1,1,'2020-05-05 02:27:36',0,0,0,0,'0',NULL,NULL,NULL,NULL,NULL,NULL,0,0,64,64,16777343,0),(133,'192.168.128.122',2055252160,NULL,NULL,NULL,'sadmin','sadmin','2021-05-08 06:46:24','系统安装时自动添加',0,0,0,1,1,'2021-05-08 06:15:00',1620456344,1620456369,1620456384,1620456243,'May  7 2021 12:35:24','open xrkmon','CentOS','GLIBC_2.17','GLIBCXX_3.4.19','x86_64',NULL,0,2055252160,64,64,2055252160,1);
/*!40000 ALTER TABLE `mt_machine` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_machine_ins after insert on mt_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_machine', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_machine_up after update on mt_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_machine', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_module_info`
--

DROP TABLE IF EXISTS `mt_module_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_module_info` (
  `module_name` varchar(32) NOT NULL,
  `module_desc` varchar(128) NOT NULL,
  `app_id` int(11) unsigned DEFAULT '0',
  `module_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `xrk_status` tinyint(4) DEFAULT '0',
  `user_add` varchar(32) NOT NULL,
  `user_mod` varchar(32) NOT NULL,
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`module_id`)
) ENGINE=InnoDB AUTO_INCREMENT=151 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_module_info`
--

LOCK TABLES `mt_module_info` WRITE;
/*!40000 ALTER TABLE `mt_module_info` DISABLE KEYS */;
INSERT INTO `mt_module_info` VALUES ('slog_config','处理迁移云管系统相关配置，有更新时将配置从数据库更新到共享内存',30,21,0,'sadmin','sadmin','2019-08-26 12:50:24',1,1,'0000-00-00 00:00:00'),('slog_server','日志服务，接收远程log并写入服务器共享内存中',30,22,0,'sadmin','sadmin','2019-08-26 12:50:17',1,1,'0000-00-00 00:00:00'),('slog_write','日志写入磁盘服务',30,23,0,'sadmin','sadmin','2019-08-26 12:50:12',1,1,'0000-00-00 00:00:00'),('slog_client','用于收集多机部署时，迁移云管系统自身产生的日志',30,24,0,'sadmin','sadmin','2019-08-26 12:50:07',1,1,'0000-00-00 00:00:00'),('fcgi_slog_flogin','用于处理用户登录授权',30,25,0,'sadmin','sadmin','2017-07-28 01:19:56',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog','用于处理系统日志展示，应用模块配置等',30,26,0,'sadmin','sadmin','2014-12-14 04:04:29',1,1,'0000-00-00 00:00:00'),('slog_mtreport_server','管理监控 agent slog_mtreport_client 的接入，以及下发配置',30,27,0,'sadmin','sadmin','2019-08-26 12:44:40',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_monitor','迁移云管系统主页',30,28,0,'sadmin','sadmin','2019-07-02 07:53:44',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_attr','用于管理监控点和监控点类型',30,29,0,'sadmin','sadmin','2019-08-26 12:43:33',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_machine','用于管理系统机器配置',30,30,0,'sadmin','sadmin','2019-07-02 07:51:56',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_view','用于处理视图配置',30,31,0,'sadmin','sadmin','2019-07-02 07:51:44',1,1,'0000-00-00 00:00:00'),('slog_monitor_server','用于处理监控点上报',30,33,0,'sadmin','sadmin','2019-08-26 12:12:09',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_showview','处理web系统视图展示',30,34,0,'sadmin','sadmin','2018-05-23 11:11:49',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_warn','提示配置',30,35,0,'sadmin','sadmin','2019-08-31 11:07:39',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_user','迁移云管系统用户管理cgi',30,39,0,'sadmin','sadmin','2019-08-31 11:07:26',1,1,'0000-00-00 00:00:00'),('slog_monitor_client','迁移云管系统本身的监控点上报服务',30,47,0,'sadmin','sadmin','2019-08-26 12:11:46',1,1,'0000-00-00 00:00:00'),('slog_deal_warn','迁移提示处理模块',30,48,0,'sadmin','sadmin','2019-08-26 12:11:00',1,1,'0000-00-00 00:00:00'),('slog_check_warn','提示检查模块',30,59,0,'sadmin','sadmin','2019-08-26 12:11:26',1,1,'0000-00-00 00:00:00'),('fcgi_mt_slog_reportinfo','用于公共上报的 fastcgi 模块',30,150,0,'sadmin','sadmin','2020-05-04 22:18:17',1,1,'2020-05-04 22:18:17');
/*!40000 ALTER TABLE `mt_module_info` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_module_info_ins after insert on mt_module_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_module_info', new.module_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_module_info_up after update on mt_module_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_module_info', old.module_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_plugin`
--

DROP TABLE IF EXISTS `mt_plugin`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_plugin` (
  `plugin_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `xrk_status` tinyint(4) DEFAULT '0',
  `create_time` int(12) unsigned DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT '1970-01-01 16:00:00' ON UPDATE CURRENT_TIMESTAMP,
  `pb_info` text,
  `open_plugin_id` int(11) unsigned NOT NULL,
  `plugin_name` varchar(64) NOT NULL,
  `plugin_show_name` varchar(128) NOT NULL,
  PRIMARY KEY (`plugin_id`),
  KEY `open_id_key` (`open_plugin_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_plugin`
--

LOCK TABLES `mt_plugin` WRITE;
/*!40000 ALTER TABLE `mt_plugin` DISABLE KEYS */;
/*!40000 ALTER TABLE `mt_plugin` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `mt_plugin_machine`
--

DROP TABLE IF EXISTS `mt_plugin_machine`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_plugin_machine` (
  `xrk_id` int(11) NOT NULL AUTO_INCREMENT,
  `machine_id` int(11) NOT NULL,
  `last_attr_time` int(12) unsigned DEFAULT '0',
  `last_log_time` int(12) unsigned DEFAULT '0',
  `start_time` int(12) unsigned DEFAULT '0',
  `lib_ver_num` int(11) DEFAULT '0',
  `last_hello_time` int(12) unsigned DEFAULT '0',
  `xrk_status` int(11) DEFAULT '0',
  `install_proc` int(11) DEFAULT '0',
  `open_plugin_id` int(11) unsigned NOT NULL,
  `local_cfg_url` varchar(1024) DEFAULT NULL,
  `down_opr_cmd` int(11) unsigned DEFAULT '0',
  `opr_proc` int(6) unsigned DEFAULT '0',
  `opr_start_time` int(12) unsigned DEFAULT '0',
  `cfg_file_time` int(12) unsigned DEFAULT '0',
  `xrk_cfgs_list` text,
  `down_cfgs_list` text,
  `down_cfgs_restart` int(4) DEFAULT '0',
  `start_attr_time` int(12) unsigned DEFAULT '0',
  `plugin_version` char(16) NOT NULL DEFAULT 'v1.0.0',
  `inner_mode_install` int(4) DEFAULT '0',
  PRIMARY KEY (`xrk_id`),
  UNIQUE KEY `m_p` (`machine_id`,`open_plugin_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_plugin_machine`
--

LOCK TABLES `mt_plugin_machine` WRITE;
/*!40000 ALTER TABLE `mt_plugin_machine` DISABLE KEYS */;
/*!40000 ALTER TABLE `mt_plugin_machine` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_plugin_machine_ins after insert on mt_plugin_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_plugin_machine', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_plugin_machine_up after update on mt_plugin_machine for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_plugin_machine', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_server`
--

DROP TABLE IF EXISTS `mt_server`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_server` (
  `ip` char(20) NOT NULL,
  `xrk_port` int(11) unsigned NOT NULL DEFAULT '12345',
  `xrk_type` int(8) unsigned NOT NULL DEFAULT '0',
  `xrk_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `sand_box` int(8) unsigned DEFAULT '0',
  `region` int(8) unsigned DEFAULT '0',
  `idc` int(8) unsigned DEFAULT '0',
  `xrk_status` tinyint(4) DEFAULT '0',
  `srv_for` text,
  `weight` int(11) unsigned DEFAULT '60000',
  `cfg_seq` int(11) unsigned DEFAULT '1',
  `user_add` varchar(64) NOT NULL,
  `user_mod` varchar(64) NOT NULL,
  `create_time` int(12) unsigned DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `m_desc` varchar(256) DEFAULT NULL,
  PRIMARY KEY (`ip`,`xrk_type`),
  KEY `id` (`xrk_id`)
) ENGINE=InnoDB AUTO_INCREMENT=25 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_server`
--

LOCK TABLES `mt_server` WRITE;
/*!40000 ALTER TABLE `mt_server` DISABLE KEYS */;
INSERT INTO `mt_server` VALUES ('192.168.128.122',28080,1,3,0,0,0,0,'30,119',1000,1584529976,'sadmin','sadmin',1474811820,'2021-05-08 06:15:00','绑定应用id，处理日志上报，可部署多台'),('192.168.128.122',38080,2,6,0,0,0,0,'',1000,1567504630,'sadmin','sadmin',1475408172,'2021-05-08 06:15:00','处理监控点数据上报、可部署多台'),('192.168.128.122',3306,3,4,0,0,0,0,'',1000,1567577833,'sadmin','sadmin',1475152894,'2021-05-08 06:15:00','mysql 监控点服务器，部署1台'),('192.168.128.122',27000,4,24,0,0,0,0,'中心服务器用于归总各服务器都有上报的数据，属于 slog_mtreport_server 中的某一台',1000,1589376081,'sadmin','sadmin',0,'2021-05-08 06:15:00','slog_mtreport_server 中的一台'),('192.168.128.122',12121,11,23,0,0,0,0,'',1000,1567504613,'sadmin','sadmin',1561962711,'2021-05-08 06:15:00','web 控制台服务器，部署1台');
/*!40000 ALTER TABLE `mt_server` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_server_ins after insert on mt_server for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_server', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_server_up after update on mt_server for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_server', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_table_upate_monitor`
--

DROP TABLE IF EXISTS `mt_table_upate_monitor`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_table_upate_monitor` (
  `u_table_name` varchar(32) NOT NULL,
  `r_primary_id` int(11) unsigned DEFAULT '0',
  `r_primary_id_2` int(11) unsigned DEFAULT '0',
  `create_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `r_change_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`r_change_id`)
) ENGINE=MyISAM AUTO_INCREMENT=239132 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_table_upate_monitor`
--

LOCK TABLES `mt_table_upate_monitor` WRITE;
/*!40000 ALTER TABLE `mt_table_upate_monitor` DISABLE KEYS */;
/*!40000 ALTER TABLE `mt_table_upate_monitor` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `mt_view`
--

DROP TABLE IF EXISTS `mt_view`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_view` (
  `xrk_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `xrk_name` varchar(64) NOT NULL,
  `user_add` varchar(64) DEFAULT 'sadmin' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `user_mod` varchar(64) DEFAULT 'sadmin' COMMENT 'æœ€åŽæ›´æ–°ç”¨æˆ·',
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'æ›´æ–°æ—¶é—´',
  `view_desc` varchar(256) DEFAULT NULL COMMENT 'è§†å›¾æè¿°',
  `xrk_status` tinyint(4) DEFAULT '0',
  `view_flag` int(8) DEFAULT '0' COMMENT 'å‘Šè­¦æ ‡è®°',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`xrk_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1000032 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_view`
--

LOCK TABLES `mt_view` WRITE;
/*!40000 ALTER TABLE `mt_view` DISABLE KEYS */;
INSERT INTO `mt_view` VALUES (22,'迁移云管系统-常用','sadmin','sadmin','2019-05-23 03:17:30','迁移云管系统常用上报',0,1,1,1,'0000-00-00 00:00:00'),(23,'日志系统-数据','sadmin','sadmin','2019-07-03 03:43:57','一些重要的日志系统数据属性上报',0,1,1,1,'0000-00-00 00:00:00'),(26,'cgi 监控','sadmin','sadmin','2019-05-23 03:17:00','cgi相关监控上报',0,1,1,1,'0000-00-00 00:00:00');
/*!40000 ALTER TABLE `mt_view` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_ins after insert on mt_view for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_view', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_up after update on mt_view for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_view', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_view_battr`
--

DROP TABLE IF EXISTS `mt_view_battr`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_view_battr` (
  `view_id` int(11) NOT NULL DEFAULT '0' COMMENT 'è§†å›¾ç¼–å·',
  `attr_id` int(11) NOT NULL DEFAULT '0' COMMENT 'å±žæ€§ç¼–å·',
  `xrk_status` tinyint(4) DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`view_id`,`attr_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_view_battr`
--

LOCK TABLES `mt_view_battr` WRITE;
/*!40000 ALTER TABLE `mt_view_battr` DISABLE KEYS */;
INSERT INTO `mt_view_battr` VALUES (21,62,0,'2019-11-17 11:34:44'),(21,74,0,'2019-11-17 11:34:44'),(21,63,0,'2019-11-17 11:34:44'),(21,56,0,'2019-11-17 11:34:44'),(21,55,0,'2019-11-17 11:34:44'),(21,57,0,'2019-11-17 11:34:44'),(21,64,0,'2019-11-17 11:34:44'),(21,71,0,'2019-11-17 11:34:44'),(21,73,0,'2019-11-17 11:34:44'),(21,58,0,'2019-11-17 11:34:44'),(21,65,0,'2019-11-17 11:34:44'),(23,70,0,'2019-11-17 11:34:44'),(26,113,0,'2019-11-17 11:34:44'),(23,65,0,'2019-11-17 11:34:44'),(23,93,0,'2019-11-17 11:34:44'),(21,67,0,'2019-11-17 11:34:44'),(21,75,0,'2019-11-17 11:34:44'),(21,59,0,'2019-11-17 11:34:44'),(21,60,0,'2019-11-17 11:34:44'),(21,70,0,'2019-11-17 11:34:44'),(23,73,0,'2019-11-17 11:34:44'),(23,75,0,'2019-11-17 11:34:44'),(21,61,0,'2019-11-17 11:34:44'),(21,66,0,'2019-11-17 11:34:44'),(21,68,0,'2019-11-17 11:34:44'),(21,69,0,'2019-11-17 11:34:44'),(21,72,0,'2019-11-17 11:34:44'),(21,76,0,'2019-11-17 11:34:44'),(23,67,0,'2019-11-17 11:34:44'),(21,77,0,'2019-11-17 11:34:44'),(21,78,0,'2019-11-17 11:34:44'),(21,79,0,'2019-11-17 11:34:44'),(21,80,0,'2019-11-17 11:34:44'),(21,81,0,'2019-11-17 11:34:44'),(21,82,0,'2019-11-17 11:34:44'),(21,83,0,'2019-11-17 11:34:44'),(26,143,0,'2019-11-17 11:34:44'),(26,112,0,'2019-11-17 11:34:44'),(26,111,0,'2019-11-17 11:34:44'),(26,110,0,'2019-11-17 11:34:44'),(26,146,0,'2019-11-17 11:34:44'),(26,147,0,'2019-11-17 11:34:44'),(26,145,0,'2019-11-17 11:34:44'),(26,108,0,'2019-11-17 11:34:44'),(26,144,0,'2019-11-17 11:34:44'),(26,142,0,'2019-11-17 11:34:44'),(23,62,0,'2019-11-17 11:34:44'),(23,94,0,'2019-11-17 11:34:44'),(22,67,0,'2019-11-17 11:34:44'),(22,73,0,'2019-11-17 11:34:44');
/*!40000 ALTER TABLE `mt_view_battr` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_battr_ins after insert on mt_view_battr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id, r_primary_id_2) values('mt_view_battr', new.view_id, new.attr_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_battr_up after update on mt_view_battr for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id, r_primary_id_2) values('mt_view_battr', old.view_id, old.attr_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_view_bmach`
--

DROP TABLE IF EXISTS `mt_view_bmach`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_view_bmach` (
  `view_id` int(11) NOT NULL DEFAULT '0' COMMENT 'è§†å›¾ç¼–å·',
  `machine_id` int(11) NOT NULL DEFAULT '0' COMMENT 'æœºå™¨ç¼–å·',
  `xrk_status` tinyint(4) DEFAULT '0',
  `user_master` int(11) unsigned DEFAULT '1',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`view_id`,`machine_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_view_bmach`
--

LOCK TABLES `mt_view_bmach` WRITE;
/*!40000 ALTER TABLE `mt_view_bmach` DISABLE KEYS */;
INSERT INTO `mt_view_bmach` VALUES (23,133,0,1,'2021-05-08 06:16:09'),(22,133,0,1,'2021-05-08 06:16:09'),(26,133,0,1,'2021-05-08 06:23:09');
/*!40000 ALTER TABLE `mt_view_bmach` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_bmach_ins after insert on mt_view_bmach for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id, r_primary_id_2) values('mt_view_bmach', new.view_id, new.machine_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_view_bmach_up after update on mt_view_bmach for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id, r_primary_id_2) values('mt_view_bmach', old.view_id, old.machine_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_warn_config`
--

DROP TABLE IF EXISTS `mt_warn_config`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_warn_config` (
  `warn_config_id` int(11) NOT NULL AUTO_INCREMENT COMMENT 'å‘Šè­¦é…ç½®ç¼–å·',
  `attr_id` int(11) DEFAULT NULL COMMENT 'å±žæ€§ç¼–å·',
  `warn_flag` int(11) DEFAULT '0' COMMENT 'å‘Šè­¦æ ‡è®°',
  `max_value` int(11) DEFAULT '0' COMMENT 'æœ€å¤§å€¼',
  `min_value` int(11) DEFAULT '0' COMMENT 'æœ€å°å€¼',
  `wave_value` int(11) DEFAULT '0' COMMENT 'æ³¢åŠ¨å€¼',
  `warn_type_value` int(11) DEFAULT '0' COMMENT 'å‘Šè­¦ç±»åž‹å€¼',
  `user_add` varchar(64) DEFAULT 'sadmin' COMMENT 'æ·»åŠ ç”¨æˆ·',
  `reserved1` int(11) DEFAULT '0' COMMENT 'æœºå™¨id æˆ–è€… è§†å›¾id',
  `reserved2` int(11) DEFAULT '0' COMMENT 'ä¿ç•™',
  `reserved3` varchar(32) DEFAULT NULL COMMENT 'ä¿ç•™',
  `reserved4` varchar(32) DEFAULT NULL COMMENT 'ä¿ç•™',
  `xrk_status` tinyint(4) DEFAULT '0',
  `user_add_id` int(11) unsigned DEFAULT '1',
  `user_mod_id` int(11) unsigned DEFAULT '1',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `create_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`warn_config_id`)
) ENGINE=MyISAM AUTO_INCREMENT=120 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_warn_config`
--

LOCK TABLES `mt_warn_config` WRITE;
/*!40000 ALTER TABLE `mt_warn_config` DISABLE KEYS */;
/*!40000 ALTER TABLE `mt_warn_config` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_warn_config_ins after insert on mt_warn_config for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_warn_config', new.warn_config_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = latin1 */ ;
/*!50003 SET character_set_results = latin1 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_warn_config_up after update on mt_warn_config for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_warn_config', old.warn_config_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `mt_warn_info`
--

DROP TABLE IF EXISTS `mt_warn_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mt_warn_info` (
  `wid` int(11) NOT NULL AUTO_INCREMENT,
  `warn_id` int(11) DEFAULT '0',
  `attr_id` int(11) DEFAULT '0',
  `warn_config_val` int(11) DEFAULT '0',
  `warn_val` int(11) DEFAULT '0',
  `warn_time_utc` int(11) unsigned DEFAULT NULL,
  `warn_flag` int(11) DEFAULT '0',
  `deal_status` int(11) DEFAULT '0',
  `last_warn_time_utc` int(11) unsigned DEFAULT NULL,
  `warn_times` int(11) DEFAULT '0',
  `send_warn_times` int(11) DEFAULT '0',
  `start_deal_time_utc` int(11) unsigned DEFAULT NULL,
  `end_deal_time_utc` int(11) unsigned DEFAULT NULL,
  `xrk_status` tinyint(4) DEFAULT '0',
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`wid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `mt_warn_info`
--

LOCK TABLES `mt_warn_info` WRITE;
/*!40000 ALTER TABLE `mt_warn_info` DISABLE KEYS */;
/*!40000 ALTER TABLE `mt_warn_info` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_warn_info_ins after insert on mt_warn_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_warn_info', new.wid); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger mt_warn_info_up after update on mt_warn_info for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('mt_warn_info', old.wid); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;

--
-- Table structure for table `test_key_list`
--

DROP TABLE IF EXISTS `test_key_list`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `test_key_list` (
  `config_id` int(10) unsigned NOT NULL DEFAULT '0',
  `test_key` varchar(128) NOT NULL,
  `test_key_type` int(10) unsigned DEFAULT '0',
  `xrk_status` tinyint(4) DEFAULT '0',
  `xrk_id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `update_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`xrk_id`),
  KEY `config_id` (`config_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `test_key_list`
--

LOCK TABLES `test_key_list` WRITE;
/*!40000 ALTER TABLE `test_key_list` DISABLE KEYS */;
/*!40000 ALTER TABLE `test_key_list` ENABLE KEYS */;
UNLOCK TABLES;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger test_key_list_ins after insert on test_key_list for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('test_key_list', new.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
ALTER DATABASE `mtreport_db` CHARACTER SET utf8 COLLATE utf8_general_ci ;
/*!50003 SET @saved_cs_client      = @@character_set_client */ ;
/*!50003 SET @saved_cs_results     = @@character_set_results */ ;
/*!50003 SET @saved_col_connection = @@collation_connection */ ;
/*!50003 SET character_set_client  = utf8 */ ;
/*!50003 SET character_set_results = utf8 */ ;
/*!50003 SET collation_connection  = utf8_general_ci */ ;
/*!50003 SET @saved_sql_mode       = @@sql_mode */ ;
/*!50003 SET sql_mode              = 'STRICT_TRANS_TABLES,NO_ENGINE_SUBSTITUTION' */ ;
DELIMITER ;;
/*!50003 CREATE*/ /*!50017 DEFINER=`root`@`localhost`*/ /*!50003 trigger test_key_list_up after update on test_key_list for each row begin insert into mt_table_upate_monitor(u_table_name, r_primary_id) values('test_key_list', old.xrk_id); end */;;
DELIMITER ;
/*!50003 SET sql_mode              = @saved_sql_mode */ ;
/*!50003 SET character_set_client  = @saved_cs_client */ ;
/*!50003 SET character_set_results = @saved_cs_results */ ;
/*!50003 SET collation_connection  = @saved_col_connection */ ;
ALTER DATABASE `mtreport_db` CHARACTER SET latin1 COLLATE latin1_swedish_ci ;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2021-05-07 23:48:08
