/*** DES-PWE license ***

   Copyright (c) 2019 by itc

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   DES-PWE migration (c) 2023 by itc
   current version：v1.0
   License Agreement： apache license 2.0

   

   云版本为开源版提供永久免费提示通道支持，提示通道支持短信、邮件、
   微信等多种方式，欢迎使用

****/

var g_dmtChartColor = ['#FF4949','#FFA74D','#FFEA51','#4BF0FF','#44AFF0','#4E82FF','#584BFF','#BE4DFF','#F845F1'];
var g_dmtChartStyle = 'dark';
var g_dmtChartWidth;
var g_dmtLastLeftShow = null;
var g_dmtLastType = null;
var g_dmtLastTypeId = null;
var g_dmtChartMargin = 8;
var g_dmtChartMinWidth = 460;
var g_dmtChartHeight = 320;
var g_dmtSingleChartMinWidth = 1100;
var g_dmtSingleChartHeight = 520;
var g_all_charts = new Array();
var g_dmtRedrawChartCountShowProc = 5;

var g_attr_type_list = null;
var g_PluginMargin = 10;

// 监控点值类型
var ATTR_VALUE_TYPE_COMMON=0,
    ATTR_VALUE_TYPE_PERCENT=1,
    ATTR_VALUE_TYPE_DIVISION_100=2;
    ATTR_VALUE_TYPE_MULTI_1024=3;

// 监控点图表绘制类型
var ATTR_CHART_TYPE_LINE=0,
    ATTR_CHART_TYPE_LINE_AREA=1,
    ATTR_CHART_TYPE_BAR=6,
    ATTR_CHART_TYPE_PIE=12,
    ATTR_CHART_TYPE_SCATTER=18,
    ATTR_CHART_TYPE_GEO=24,
    ATTR_CHART_TYPE_GRAPH=30,
    ATTR_CHART_TYPE_TREE=36,
    ATTR_CHART_TYPE_TREEMAP=42,
    ATTR_CHART_TYPE_FUNNEL=48,
    ATTR_CHART_TYPE_GAUGE=54;

var SUM_REPORT_M=1, // 累计量
    SUM_REPORT_MIN=2, // 取统计周期内上报的最小值
    EX_REPORT=3, // 异常量
    SUM_REPORT_MAX=4, // 取统计周期内上报的最大值
    SUM_REPORT_TOTAL = 5, // 累计所有的上报数据
    STR_REPORT_D = 6, // 按天累计的字符串型, 一天生
    STR_REPORT_D_IP = 7, // 字符串型 - IP省级
    DATA_USE_LAST = 8; // 使用最新的上报数据

function dmtMathtrunc(d) {
    if(typeof Math.trunc != 'function') 
        return Math.round(d);
    return Math.trunc(d);   
}

function dmtGetHumanTime(val)
{
    var h = '';
    if(val >= 86400) {
        h += dmtMathtrunc(val/86400) + '天 ';
        val %= 86400;
    }
    if(val >= 3600) {
        h += dmtMathtrunc(val/3600) + '小时 ';
        val %= 3600;
    }
    if(val >= 60) {
        h += dmtMathtrunc(val/60) + '分钟 ';
        val %= 60;
    }
    if(val >= 1) {
        h += dmtMathtrunc(val) + '秒 ';
    }
    return h;
}

var g_pluginShowWidth = 0;
var g_pluginShowHeight = 0;
function dmtSetPluginShowWH()
{
    var iSub = DWZ.ui.sbar ? $("#sidebar").width() : 35;
    var iContentW = $(window).width() - iSub;
    var bChanged = false;
    if(iContentW != g_pluginShowWidth) {
        g_pluginShowWidth = iContentW;
        bChanged = true;
    }

	var iContentH = $(window).height() - $('header').outerHeight(true) 
        - $('footer').outerHeight(true) - $('.tabsPageHeader').outerHeight(true);
    if(g_pluginShowHeight != iContentH) {
        g_pluginShowHeight = iContentH;
        bChanged = true;
    }
    return bChanged;
}

function dmtSetPluginMarginInfo(tab)
{
	var iSub = DWZ.ui.sbar ? $("#sidebar").width() + 10 : 24;
	var iContentW = $(window).width() - iSub - 20 - 45;
	var xLeft = iContentW % 452;
	var xCount = dmtMathtrunc(iContentW/452);
	var newM = dmtMathtrunc(xLeft/xCount/2);
	if(newM != g_PluginMargin) {
		g_PluginMargin = newM;
		if(typeof tab == 'undefined')
			return true;

		var ptype = '';
		if(tab == "dmt_plugin_open")
			ptype='#dpopen_plugin_list';
		else {
			dmtJsBugReport('dmt.comm.js', 'dmtSetPluginMarginInfo', 'unknow plugin tab:'+tab);
			return true;
		}

		$(ptype).children().each(function(){
			$(this).css('margin', '10px '+g_PluginMargin+'px');
		});
		return true;
	}
	return false;
}

function dmtSetChartOptionByAttrInfo(attr_info, op, extra)
{
    var showtype = '';
    var day_type = 0;
    if(typeof extra.showtype != 'undefined')
        showtype = extra.showtype;
    if(typeof extra.day_type != 'undefined')
        day_type = extra.day_type;

    // 图表类型
    if(attr_info.chart_type == ATTR_CHART_TYPE_BAR) {
        op.series[0].type = 'bar';
        if(day_type == 1) {
            op.series[1].type = 'bar';
            op.series[2].type = 'bar';
        }
    }
    else if(attr_info.chart_type == ATTR_CHART_TYPE_LINE_AREA) {
        op.series[0].type = 'line';
        op.series[0].areaStyle = {};
        op.series[0].stack = 'stack_line';

        if(day_type == 1) {
            op.series[1].type = 'line';
            op.series[1].areaStyle = {};
            op.series[1].stack = 'stack_line';
            op.series[2].type = 'line';
            op.series[2].areaStyle = {};
            op.series[2].stack = 'stack_line';
        }
    }
    else {
        op.series[0].type = 'line';
        op.series[0].areaStyle = null;
        op.series[0].stack = null;
        if(day_type == 1) {
            op.series[1].type = 'line';
            op.series[1].areaStyle = null;
            op.series[1].stack = null;
            op.series[2].type = 'line';
            op.series[2].areaStyle = null;
            op.series[2].stack = null;
        }
    }

    // Y 轴值类型
    if(attr_info.value_type == ATTR_VALUE_TYPE_PERCENT) {
        op.yAxis.axisLabel.formatter = '{value}%';
        op.tooltip.formatter = function(p, t, ck) {
            var h = p[0].axisValueLabel + '<br />' + p[0].seriesName + ': ' + p[0].value[1] + '%';
            if(p.length == 3) {
                // 同比图
                h += '<br />' + p[1].seriesName + ': ' + p[1].value[1] + '%';
                h += '<br />' + p[2].seriesName + ': ' + p[2].value[1] + '%';
            }
            return h;
        }
    }
    else if(attr_info.value_type == ATTR_VALUE_TYPE_DIVISION_100) {
        op.yAxis.axisLabel.formatter = function (value, index) { 
            var f = value/100;
            return f.toFixed(2);
        };
        op.tooltip.formatter = function(p, t, ck) {
            var f = p[0].value[1]/100;
            var h = p[0].axisValueLabel + '<br />' + p[0].seriesName + ': ' + f.toFixed(2);
            if(p.length == 3) {
                // 同比图
                h += '<br />' + p[1].seriesName + ': ' + (p[1].value[1]/100).toFixed(2);
                h += '<br />' + p[2].seriesName + ': ' + (p[2].value[1]/100).toFixed(2);
            }
            return h;
 
        }
    }
    else if(attr_info.value_type == ATTR_VALUE_TYPE_MULTI_1024) {
        op.yAxis.axisLabel.formatter = function (value, index) { 
            return dmtGetHumanReadFixed2ByKB(value);
        };
        op.tooltip.formatter = function(p, t, ck) {
            var h = p[0].axisValueLabel + '<br />' + p[0].seriesName + ': ' + dmtGetHumanReadFixed2ByKB(p[0].value[1]);
            if(p.length == 3) {
                // 同比图
                h += '<br />' + p[1].seriesName + ': ' + dmtGetHumanReadFixed2ByKB(p[1].value[1]);
                h += '<br />' + p[2].seriesName + ': ' + dmtGetHumanReadFixed2ByKB(p[2].value[1]);
            }
            return h;
        }
    }
    else  {
        op.yAxis.axisLabel.formatter = function (value, index) { 
			if(showtype == 'site')
				return value;
            return dmtGetHumanReadFixed2(value);
	  	};
        op.tooltip.formatter = function(p, t, ck) {
            var h = p[0].axisValueLabel + '<br />' + p[0].seriesName + ': ' + dmtGetHumanReadFixed2(p[0].value[1]);
            if(p.length == 3) {
                // 同比图
                h += '<br />' + p[1].seriesName + ': ' + dmtGetHumanReadFixed2(p[1].value[1]);
                h += '<br />' + p[2].seriesName + ': ' + dmtGetHumanReadFixed2(p[2].value[1]);
            }
            return h;
        }
    }
}

// 判断浏览器类型是否支持
function dmtIsExplorerSupport()
{
	//var ua = navigator.userAgent.toLowerCase();
	//if(ua.indexOf("firefox") == -1 && ua.indexOf("chrome") == -1)
	//	return false;
	return true;
}

function dmtGetCookie(name)
{
	var arr = document.cookie.match(new RegExp("(^| )"+name+"=([^;]*)(;|$)"));
	if(arr != null) return unescape(arr[2]); return null;
}

function dmtCheckFull() 
{
	var isFull = document.fullscreenElement || document.mozFullScreenElement || document.webkitFullscreenElement;
	if (isFull == null) 
		return false;
	return isFull;
}

function dmtToggleFullScreen(element) 
{  
	var full = '';
	if(dmtCheckFull()) {
		var exitMethod = document.exitFullscreen || //W3C
            document.mozCancelFullScreen || //FireFox
            document.webkitExitFullscreen || //Chrome等
            document.webkitExitFullscreen; //IE11
        if (exitMethod) {
            exitMethod.call(document);
        } else if (typeof window.ActiveXObject !== "undefined") { //for Internet Explorer
            var wscript = new ActiveXObject("WScript.Shell");
            if (wscript !== null) {
                wscript.SendKeys("{F11}");
            }
        }
		full = 'no';
	}
	else {
		var requestMethod = element.requestFullScreen || //W3C
			element.webkitRequestFullScreen || //FireFox
			element.mozRequestFullScreen || //Chrome等
			element.msRequestFullScreen; //IE11
		full = 'error';
		if (requestMethod) {
			requestMethod.call(element);
			full = 'yes';
		} else if (typeof window.ActiveXObject !== "undefined") { //for Internet Explorer
			var wscript = new ActiveXObject("WScript.Shell");
			if (wscript !== null) {
				wscript.SendKeys("{F11}");
				full = 'yes';
			}
		}
	}
	return full;
}  


// 清除 cookie
function dmtDelCookie(name)
{
	var exp = 7*24*60*60*1000;  
	var date = new Date(+new Date()-exp);
	var cval = dmtGetCookie(name);
	if(cval != null)
		document.cookie = name + "=" + cval + "; expires="+ date.toUTCString() + ";path=/";
}

// 计算图表展示区的图表的尺寸，图表尺寸最小为 620*500
function dmtSetChartSize()
{
	// 45 -- 预留给滚动条
	var iSub = DWZ.ui.sbar ? $("#sidebar").width() + 10 : 24;
	var iContentW = $(window).width() - iSub - 5 - 45;
	if(typeof g_dmtLastLeftShow != 'undefined' && g_dmtLastLeftShow)
		iContentW -= $('.chartLeftMenu').width();

	var xWidth = g_dmtChartMinWidth+g_dmtChartMargin*2;
	var xLeft = iContentW % xWidth;
	var xCount = dmtMathtrunc(iContentW/xWidth);
	var xBlank = dmtMathtrunc(xLeft/xCount);
	g_dmtChartWidth = g_dmtChartMinWidth+xBlank;
}

function dmtSetRedrawChartsInfo(type_id, type, isLeftShow)
{
	g_dmtLastTypeId = type_id;
	g_dmtLastType = type;
	g_dmtLastLeftShow = isLeftShow;
}

// 重新计算图表尺寸, 重绘图表
function dmtRedrawCharts(type_id, type, isLeftShow)
{
	if(typeof type_id != 'undefined')
	{
		g_dmtLastTypeId = type_id;
		g_dmtLastType = type;
		g_dmtLastLeftShow = isLeftShow;
	}
	else {
		type_id = g_dmtLastTypeId;
		type = g_dmtLastType;
		isLeftShow = g_dmtLastLeftShow;
	}
	if(type_id == null) 
		return;

    if(type == 'plugin') {
        if(dmtSetPluginShowWH()) {
            var js = "if(typeof dmtPluginShowOnChane_" + type_id + " == 'function') ";
            js += "dmtPluginShowOnChane_" + type_id + "();";
            eval(js);
        }
        return;
    }

	dmtSetChartSize();
	if(typeof g_all_charts != 'undefined')
	{
		var keystr = '_'+type_id+type;
		var iNeedRedraw = 0;
		for(ct in g_all_charts) {
            if(typeof ct == 'string' && ct.match(keystr)) {
				if(typeof g_all_charts[ct].setwidth != 'undefined' && g_all_charts[ct].setwidth == g_dmtChartWidth) 
					return; 
				iNeedRedraw++; 
			}
		}

		var rchart = $("#my_background,#my_progressBar"); 
		if(iNeedRedraw > g_dmtRedrawChartCountShowProc) { 
			$('#my_progressBar font').text('图表重绘中, 请稍等...'); 
			rchart.show(); 
		}

		setTimeout(function() {
			for(ct in g_all_charts)
			{
				if(ct.match(keystr)) 
				{
					$('#'+ct).css('width', g_dmtChartWidth);
					g_all_charts[ct].resize();
					g_all_charts[ct].setwidth = g_dmtChartWidth;
				}
			}
			rchart.hide();
		}, 5);
	}
}

function dmtSetChartNewDetailInfo(result, chart_idx)
{
	if(typeof g_all_charts[chart_idx] == 'undefined'
		|| typeof g_all_charts[chart_idx].warnInfo == 'undefined')
	{
		alertMsg.error('javascript 脚本错误, 未找到图表信息:'+chart_idx);
		return;
	}

    g_all_charts[chart_idx].warnInfo.warn_flag = result.warn_flag;
    var valstr = '';
    if(g_all_charts[chart_idx].warnInfo.value_type == ATTR_VALUE_TYPE_PERCENT)
        valstr = '%';
	if(result.warn_flag & 1)
		g_all_charts[chart_idx].warnInfo.warn_max = result.warn_max + valstr;
	else
		g_all_charts[chart_idx].warnInfo.warn_max = null;

	if(result.warn_flag & 2)
		g_all_charts[chart_idx].warnInfo.warn_min = result.warn_min + valstr;
	else
		g_all_charts[chart_idx].warnInfo.warn_min = null;

	if(result.warn_flag & 4)
		g_all_charts[chart_idx].warnInfo.warn_wave = result.warn_wave + '% ';
	else
		g_all_charts[chart_idx].warnInfo.warn_wave = null;

	var op = {
		toolbox: {
			feature: {
				myUnMaskWarningData:{
					show:true
				},
				myMaskWarningData:{
					show:true
				},
				mySetWarningData:{
					show:true
				}
			}
		}
	};

	g_all_charts[chart_idx].warnInfo.warn_cfg_id = result.warn_config_id;
	if(result.warn_flag & 32)
	{
		op.toolbox.feature.myUnMaskWarningData.show = true;
		op.toolbox.feature.mySetWarningData.show = false;
		op.toolbox.feature.myMaskWarningData.show = false;
		g_all_charts[chart_idx].warnInfo.mask = 0;
	}
	else
	{
		op.toolbox.feature.myUnMaskWarningData.show = false;
		op.toolbox.feature.mySetWarningData.show = true;
		if((result.warn_flag&1) || (result.warn_flag&2) || (result.warn_flag&4)) {
			op.toolbox.feature.myMaskWarningData.show = true;
			g_all_charts[chart_idx].warnInfo.mask = 1;
		}
		else {
			op.toolbox.feature.myMaskWarningData.show = false;
			g_all_charts[chart_idx].warnInfo.mask = 0;
		}
	}
    g_all_charts[chart_idx].setOption(op);
    alertMsg.info('变更监控点提示配置成功');
}

function dmtMaskWarnConfig(warn_type, warn_type_id, attr_id)
{
	var reqpara = {};

	var cur_chart_idx = 'attr_' + attr_id + '_' + warn_type_id + warn_type;
	if(typeof g_all_charts[cur_chart_idx] == 'undefined'
		|| typeof g_all_charts[cur_chart_idx].warnInfo == 'undefined')
	{
		alertMsg.error('javascript 脚本错误, 无提示信息:' + cur_chart_idx);
		return;
	}
	var warnInfo = g_all_charts[cur_chart_idx].warnInfo;
	
	reqpara.action = "mask_warn_config";
	reqpara.mask = warnInfo.mask;
	reqpara.warn_cfg_id = warnInfo.warn_cfg_id;

	var cgi_path; 
	if(typeof g_siteInfo.cgi_path != 'undefined' && g_siteInfo.cgi_path != '')
		cgi_path = g_siteInfo.cgi_path;
	else
		cgi_path = warnInfo.cgi_path;
	var requrl = cgi_path+'mt_slog_warn';

	$.ajax({
		url:requrl, 
		data:reqpara, 
		global:false,
		success:function(result){
			if(dmtFirstDealAjaxResponse(result))
				return;
			if(result.statusCode != 200)
			{
				alertMsg.warn( DWZ.msg(result.msgid) );
				return;
			}
			dmtSetChartNewDetailInfo(result, cur_chart_idx);
		},
		dataType:'json'
	});
}

function dmtSetWarnConfig(warn_type, warn_type_id, attr_id)
{
	var cur_chart_idx = 'attr_' + attr_id + '_' + warn_type_id + warn_type;
	if(typeof g_all_charts[cur_chart_idx] == 'undefined'
		|| typeof g_all_charts[cur_chart_idx].warnInfo == 'undefined')
	{
		alertMsg.error('javascript 脚本错误, 无提示信息:' + cur_chart_idx);
		return;
	}

	var warnInfo = g_all_charts[cur_chart_idx].warnInfo;
	var cgi_path; 
	if(typeof g_siteInfo.cgi_path != 'undefined' && g_siteInfo.cgi_path != '')
		cgi_path = g_siteInfo.cgi_path;
	else
		cgi_path = warnInfo.cgi_path;
	var url = cgi_path + "/mt_slog_warn?action=chart_set_attr_warn";
	var attr_name = warnInfo.attr_name;

	url += "&warn_cfg_id=" + warnInfo.warn_cfg_id;
    url += "&value_type=" + warnInfo.value_type;
	url += "&attr_name=" + attr_name;

	url += "&warn_type_id=" + warn_type_id;
	url += "&warn_attr_id=" + attr_id;
	url += "&warn_type=" + warn_type ;

	var dlg = "dc_dlg_set_warn_" + warn_type_id + "_" + attr_id;
	var op = $.parseJSON('{"mask":true,"maxable":false,"height":260,"width":530,"resizable":false}'); 
	var title = "";
	if(warn_type == "view")
		title = "设置视图提示：视图【" + warn_type_id + "】" + "监控点【" + attr_id + "_" + attr_name + "】";
	else
		title = "设置服务器提示：服务器【" + warn_type_id + "】" + "监控点【" + attr_id + "_" + attr_name + "】";
	$.pdialog.open(url, dlg, title, op); 
}

function dmtShowPluginSingleAttr(cgi_path, cust_date, type, type_id, attr_id, plugin_show_name, attr_name)
{
	var url = cgi_path + "/mt_slog_showview?action=show_plugin_single_attr";
	url += "&plugin_id=" + type_id;
	url += "&cust_date=" + cust_date;
	url += "&type=" + type;
	url += "&attr_id=" + attr_id;
    url += "&plugin_show_name=" + plugin_show_name;
	var tid = 'pluginattr_'+attr_id;
	var stitle = "监控点【" + attr_id + "_" + attr_name + "】";
    var sthtml = '<img src="'+g_siteInfo.doc_path+'images/color_swatch.png'+'" />' + stitle;
    navTab.openTab(tid, url, { title:stitle, titleHtml:sthtml, fresh:true, global:false });
}

function dmtShowSingle(cust_date, attr_name, show_single_type, cgi_path, type, type_id, attr_id)
{
	var url = cgi_path + "/mt_slog_showview?action=show_single";
	url += "&type_id=" + type_id;
	url += "&cust_date=" + cust_date;
	url += "&show_type=" + type;
	url += "&attr_id=" + attr_id;
	url += "&show_single_type=" + show_single_type;

	var dlg = "dc_dlg_show_single_" + type_id + "_" + attr_id;

	var strOp = '{"mask":true,"maxable":false,"height":';
		strOp += g_dmtSingleChartHeight;
		strOp += ',"width":' + g_dmtSingleChartMinWidth + ',"resizable":true}';
	var op = $.parseJSON(strOp);
	var title = "";
	if(type == "view")
		title = "视图【" + type_id + "】" + "监控点【" + attr_id + "_" + attr_name + "】";
	else 
		title = "服务器【" + type_id + "】" + "监控点【" + attr_id + "_" + attr_name + "】";
	title += " - 上报数据查看";
	$.pdialog.open(url, dlg, title, op); 
}

function dmtSetShowSingleSite(op, showtype, attr_val_list, attr_val, attr_info)
{
	op.toolbox.feature.myShowSingleData.onclick = function() {
		return dmtShowSingle(
			attr_val_list.cust_date, 
			attr_info.name, 
			attr_val_list.show_single_type,
			attr_val_list.cgi_path, 
			showtype,
			attr_val_list.site_id ,
			attr_info.id
		);
	};
}

function dmtGetDateStr(d, bTime)
{
	var dt = new Date(d+new Date().getTimezoneOffset()*60*1000);
	var dtstr = '';

	if(typeof(bTime) == 'undefined') {
		dtstr = dt.getFullYear();
		if(dt.getMonth() < 9)
			dtstr += '-0'+(dt.getMonth()+1);
		else
			dtstr += '-'+(dt.getMonth()+1);
		if(dt.getDate() < 10)
			dtstr += '-0'+dt.getDate();
		else
			dtstr += '-'+dt.getDate();
	}

	if(dt.getHours() < 10)
		dtstr += ' 0'+dt.getHours();
	else
		dtstr += ' '+dt.getHours();
	if(dt.getMinutes() < 10)
		dtstr += ':0'+dt.getMinutes();
	else
		dtstr += ':'+dt.getMinutes();

	return dtstr;
}

function dmtSetViewChartInfo(showtype, attr_val_list, attr_val, attr_info, warnInfo)
{
    warnInfo.rep_max = attr_val.max;
	if(attr_val.max > 0) {
        warnInfo.rep_min = attr_val.min;
        warnInfo.rep_cur = attr_val.cur;
        warnInfo.rep_total = attr_val.total;
	}

    if(attr_val.cur > 0) {
        warnInfo.cur_date_time = attr_val_list.date_time;
        warnInfo.cur_static_val = attr_val.cur;
    }
    warnInfo.attr_statics_time = attr_info.static_time;
    warnInfo.attr_data_type = attr_info.data_type;
    warnInfo.value_type = attr_info.value_type;
    warnInfo.warn_flag = attr_val.warn_flag;
	if(attr_val.warn_flag & 1)
        warnInfo.warn_max = attr_val.warn_max
	if(attr_val.warn_flag & 2)
        warnInfo.warn_min = attr_val.warn_min;
	if(attr_val.warn_flag & 4)
        warnInfo.warn_wave = attr_val.warn_wave;
}

function dmtShowChartDetailInfo(show_type, obj_type_id, attr_id)
{
	var cur_chart_idx = 'attr_' + attr_id + '_' + obj_type_id + show_type;
	if(typeof g_all_charts[cur_chart_idx] == 'undefined'
		|| typeof g_all_charts[cur_chart_idx].warnInfo == 'undefined')
	{
		alertMsg.error('javascript 脚本错误, 无概要信息:' + cur_chart_idx);
		return;
	}

	var warnInfo = g_all_charts[cur_chart_idx].warnInfo;
    var chart_op = g_all_charts[cur_chart_idx].getOption();

    var obj = '';
    if(show_type == 'view')
        obj = '视图【'+obj_type_id+'】'+eval('ds_view_name_'+obj_type_id);
    else if(show_type == 'machine')
        obj = '服务器【'+obj_type_id+'】'+eval('dsm_machine_name_'+obj_type_id);
    else if(show_type == 'pluginview')
        obj = '插件【'+obj_type_id+'】'+eval('ds_plugin_name_'+obj_type_id);
    else
        obj = '对象【'+obj_type_id+'】';

    if(warnInfo.rep_max > 0)
        $('.dlg_chart_info_attr_data_info').text('最大值：' + dmtShowChangeValue(warnInfo.rep_max) 
            + '，最小值：' + dmtShowChangeValue(warnInfo.rep_min)
            + '，累计值：' + dmtShowChangeValue(warnInfo.rep_total));
    else
        $('.dlg_chart_info_attr_data_info').text('无上报');

    if(typeof warnInfo.cur_static_val != 'undefined')
        $('.dlg_chart_info_attr_last_static_info').text('上报时间：'+warnInfo.cur_date_time+'，上报值：'+
            warnInfo.cur_static_val);
    else {
        if(warnInfo.attr_data_type == STR_REPORT_D || warnInfo.attr_data_type == STR_REPORT_D_IP)
            $('.dlg_chart_info_attr_last_static_info').text('上报字符串数：'+warnInfo.str_count);
        else
            $('.dlg_chart_info_attr_last_static_info').text('无上报');
    }

    var winfo = '';
    if(typeof warnInfo.warn_max != 'undefined' && warnInfo.warn_max != null)
        winfo += ' 最大值：' + warnInfo.warn_max;
    if(typeof warnInfo.warn_min != 'undefined' && warnInfo.warn_min != null)
        winfo += ' 最小值：' + warnInfo.warn_min;
    if(typeof warnInfo.warn_wave != 'undefined' && warnInfo.warn_wave != null)
        winfo += ' 波动值：' + warnInfo.warn_wave;
    if(winfo != '') {
        if(warnInfo.warn_flag & 32)
            winfo += '&nbsp;&nbsp;<font color="red">提示已屏蔽</font>';
    }
    else {
        winfo = '未配置提示信息';
    }
    $('.dlg_chart_info_attr_warn_info').html(winfo);

    $('.dlg_chart_info_obj').text(obj);
    $('.dlg_chart_info_attr').text(chart_op.title[0].text+'，统计周期：'+dmtShowHumanStaticTime(warnInfo.attr_statics_time));

    var tl = '<i class="icon-file-text-alt icon-large"></i>' + chart_op.title[0].text+' - 图表概要信息';
    var op = { mask:true, maxable:false, height:260, width:500, resizable:false, drawable:true };
    $.pdialog.openLocal('dc_dlg_detail_chart_info', 'dc_dlg_detail_chart_info', tl, op);
}

function dmtSetCustToolBox(op, showtype, attr_val_list, attr_val, attr_info, warnInfo)
{
	var warn_type_id;
	if(showtype == 'view') {
		op.toolbox.feature.myShowSingleData.onclick = function() {
			return dmtShowSingle(
				attr_val_list.cust_date, 
				attr_info.name,
				attr_val_list.show_single_type,
				attr_val_list.cgi_path,
				showtype,
				attr_val_list.view_id, 
				attr_info.id
			);
		};
		warn_type_id = attr_val_list.view_id;
	}
    else if(showtype == 'pluginview') {
        op.toolbox.feature.myUnMaskWarningData.show = false;
        op.toolbox.feature.myMaskWarningData.show = false;
        op.toolbox.feature.mySetWarningData = false;
        op.toolbox.feature.myShowSingleData.show = false;
        warn_type_id = attr_val_list.plugin_id;
        op.toolbox.feature.myPluginShowAttrMach.show = true;
        op.toolbox.feature.myPluginShowAttrMach.onclick = function() {
            return dmtShowPluginSingleAttr(
				attr_val_list.cgi_path,
                attr_val_list.cust_date,
                attr_val_list.type,
                warn_type_id,
                attr_info.id,
                attr_val_list.plugin_show_name,
                attr_info.name
                );
        };
        op.toolbox.feature.myShowDetailData.onclick = function() {
            return dmtShowChartDetailInfo(
                showtype,
                warn_type_id,
                attr_info.id
                );
        };
        return ;
    }
	else {
		op.toolbox.feature.myShowSingleData.onclick = function() {
			return dmtShowSingle(
				attr_val_list.cust_date, 
				attr_info.name,
				attr_val_list.show_single_type,
				attr_val_list.cgi_path,
				showtype,
				attr_val_list.machine_id, 
				attr_info.id
			);
		}
		warn_type_id = attr_val_list.machine_id;
	};

	if(attr_val.warn_flag & 24)
		warnInfo.warn_cfg_id = attr_val.warn_config_id;
	else
		warnInfo.warn_cfg_id = 0;
	warnInfo.cgi_path = attr_val_list.cgi_path;
	warnInfo.value_type = attr_info.value_type;
	warnInfo.attr_name = attr_info.name;

	if(attr_val.warn_flag & 32) {
		op.toolbox.feature.myUnMaskWarningData.show = true;
		op.toolbox.feature.mySetWarningData.show = false;
		op.toolbox.feature.myMaskWarningData.show = false;
		warnInfo.mask = 0;
	}
	else {
		op.toolbox.feature.myUnMaskWarningData.show = false;
		op.toolbox.feature.mySetWarningData.show = true;
		if((attr_val.warn_flag&1) || (attr_val.warn_flag&2) || (attr_val.warn_flag&4)) {
			op.toolbox.feature.myMaskWarningData.show = true;
			warnInfo.mask = 1;
		}
		else {
			op.toolbox.feature.myMaskWarningData.show = false;
			warnInfo.mask = 0;
		}
	}

	op.toolbox.feature.myUnMaskWarningData.onclick = function() {
		return dmtMaskWarnConfig(
			showtype,
			warn_type_id,
			attr_info.id
		);
	};

	op.toolbox.feature.mySetWarningData.onclick = function() {
		return dmtSetWarnConfig(
			showtype,
			warn_type_id,
			attr_info.id
		);
	};

	op.toolbox.feature.myMaskWarningData.onclick = function() {
		return dmtMaskWarnConfig(
			showtype,
			warn_type_id,
			attr_info.id
		);
	};

	op.toolbox.feature.myShowDetailData.onclick = function() {
		return dmtShowChartDetailInfo(
			showtype,
			warn_type_id,
			attr_info.id
		);
	};
}

function dmtShowHumanStaticTime(t)
{
    switch(t)
    {
        case 1: return '1分钟';
        case 5: return '5分钟';
        case 10: return '10分钟';
        case 15: return '15分钟';
        case 30: return '30分钟';
        case 60: return '1小时';
        case 120: return '2小时';
        case 180: return '3小时';
    }
    return 'unknow';
}

// tstr: yyyy-MM-dd HH:mm:ss
function dmtGetTimeStamp(tstr)
{
	if(typeof tstr=="undefined" || tstr == "")
		return 0;
	var tmp = tstr.split(' ');          
	var temp = tmp[0].split('-');
	var y = temp[0] - 0;              
	var m = temp[1] - 0;             
	var d = temp[2] - 0;              

	temp = tmp[1].split(':');
	var h = temp[0] - 0;
	var mm = temp[1] - 0;
	var s = temp[2] - 0;
	return (new Date(y-0, m-1, d-0, h-0, mm-0, s-0)-0)/1000;
}

function dmtGetNaviAttrTypeTree(treeinfo)
{
	var list = '';
    if(typeof treeinfo.navi_has_type == 'undefined')
        return '';

	list = "<li><a href='#'>" + treeinfo.name + "</a>";
    list += "<ul attr_type='" + treeinfo.type + "' >";

	var listsub = treeinfo.list;
	for(var i=0; i < treeinfo.subcount && i < listsub.length; i++)
        list += dmtGetNaviAttrTypeTree(listsub[i]);

    list += "</ul>";
	list += "</li>";
	return list;
}

function dmtSetPluginViewChartNavigatioInfo(attr_type_tree, ctstr)
{
    if(attr_type_tree.subcount > 0) {
        var uhtml = '';
        for(var j=0; j < attr_type_tree.subcount && j < listsub.length; j++) {
            if(typeof listsub[j].navi_has_type != 'undefined')
                uhtml += dmtGetNaviAttrTypeTree(listsub[j]);
        }
        $('#' + ctstr + ' ul').html(uhtml);
    }
    else {
        var t = $('#'+ctstr+' .navi_chart_text').text();
        $('#'+ctstr+' .navi_chart_text').html('【<font color="gray"> 无 </font>】'+t);
    }

}

function dmtSetChartNavigatioInfo(chart_attr_types, attr_type_tree, ctstr)
{
    var iFindCount = 0;
    var listsub = attr_type_tree.list;
    for(var i=0; i < chart_attr_types.length; i++) 
    {
        for(var j=0; j < attr_type_tree.subcount && j < listsub.length; j++) 
        {
            if(dmtGetTypeInfo(listsub[j], chart_attr_types[i]) != 'null') {
                iFindCount++;
                break;
            }
        }
    }

    if(iFindCount > 0) {
        var uhtml = '';
        for(var j=0; j < attr_type_tree.subcount && j < listsub.length; j++) {
            if(typeof listsub[j].navi_has_type != 'undefined')
                uhtml += dmtGetNaviAttrTypeTree(listsub[j]);
        }
        $('#' + ctstr + ' ul').html(uhtml);
    }
    else {
        var t = $('#'+ctstr+' .navi_chart_text').text();
        $('#'+ctstr+' .navi_chart_text').html('【<font color="gray"> 无 </font>】'+t);
    }
}

function dmtEncodeHTML(str)
{
    var s = "";
    if (str.length === 0)
        return "";
    s = str.replace(/&#39;/g, "'")
        .replace(/&#160;/g, " ")
        .replace(/&#92;/g, "\\")
        .replace(/>/g, "&gt;")
        .replace(/</g, "&lt;")
        .replace(/_r_n/g, "<br>")
        .replace(/\\x0A/g, "<br>")
        .replace(/\\x2F/g, "/")
        .replace(/\\x3B/g, ";")
        .replace(/\\x22/g, "\"")
        .replace(/\\x3C/g, "<")
        .replace(/\\x26/g, "&")
        .replace(/\\x27/g, "\'")
        .replace(/\\x5C/g, "\\")
        .replace(/\\x3E/g, ">");

    return s;
}

function dmtJsBugReport(file, func, msg)
{
	var bugmsg = "Js bug report info -- file:" + file + "   ";
		bugmsg += "function:" + func + "   ";
		bugmsg += "info msg:" + msg + "   ";
}

function dmtExport(url, data)
{
	alertMsg.confirm("确实要导出这些记录吗?", {
	    okCall: function() {
			$.ajax({
				type:'post',
				url:url,
				data:{data:data},
				dataType:"json",
				global: false,
				cache: false,
				success: function(js){
					if(js.statusCode != 0)
					    alertMsg.warn(js.msg);
					else if(js.record_count == 0)
					    alertMsg.info('记录为空');
					else
					    window.location = js.file;
				},
			    error: DWZ.ajaxError
			});
		}
	});
}

function dmtSetTypeTree(treeinfo)
{
    var list = "<li><a href='#' " + "name=" + treeinfo.type + ">" + treeinfo.name + "</a>";
    if(treeinfo.subcount == 0) {
        list += "</li>";
        return list;
    }   

    list += "<ul>";
    var listsub = treeinfo.list;
    for(var i=0; i < treeinfo.subcount && i < listsub.length; i++)
        list += dmtSetTypeTree(listsub[i]);
    list += "</ul>";

    list += "</li>";
    return list;
}

// 通过监控点类型号或者类型信息
function dmtGetTypeInfo(treeinfo, type)
{
	if(treeinfo.type == type) {
        treeinfo.navi_has_type = true;
		return treeinfo;
    }

	var listsub = treeinfo.list;
	for(var i=0; i < treeinfo.subcount && i < listsub.length; i++)
	{
		var info = dmtGetTypeInfo(listsub[i], type);
		if(info != "null") {
            treeinfo.navi_has_type = true;
			return info; 
        }
	}
	return "null";
}

function dmtGetAttrTypeList(attr_list)
{
    var ary_types = new Array();
    for(var i=0,j=0; i < attr_list.count && i <  attr_list.list.length; i++) {
        for(j=0; j < ary_types.length; j++) {
            if(attr_list.list[i].attr_type == ary_types[j])
                break;
        }
        if(j >= ary_types.length)
            ary_types.push(attr_list.list[i].attr_type);
    }
    return ary_types;
}

// 根据监控点id 查找监控点信息
function dmtGetAttrInfo(attrinfo, attr_id)
{
	var listattr = attrinfo.list;
	for(var i=0; i < attrinfo.count && i < listattr.length; i++)
	{
		if(listattr[i].id == attr_id)
			return listattr[i];
	}
	return "null";
}

function dmtJumpToAttrPic(attr_show_id, ctview, callBack)
{
	// ctattr 为监控点曲线图 id
	var ctattr = '#'+attr_show_id;
	if($(ctattr).length <= 0) 
	{
		var arrStr = attr_show_id.split('_');
		if(callBack == 'click' || !$.isFunction(callBack))
		{
			alertMsg.info('监控点:' + arrStr[1] + ' 无数据上报');
			return false;
		}

		if(false == callBack(attr_show_id))
			alertMsg.info('监控点:' + arrStr[1] + ' 无数据上报');
		else
			alertMsg.info('监控点:' + arrStr[1] + ' 上报数据获取中...');
		return false;
	}

	// 当前滚动条位置
	var sctop = $(ctview).scrollTop();
	// 容器起始偏移
	var viewoff = $(ctview).offset().top;
	// 当前容器偏移
	var attroff = $(ctattr).offset().top;
	// 当前容器置顶的偏移量
	var off = attroff - viewoff + sctop;
	$(ctview).animate({ scrollTop: off });
	return false;
}

// 机器、视图绑定的监控点列表
function dmtSetAttrList(attrinfo, ds_attr_type_list, type, attr_list_arry, clickCallBack)
{
	var listattr = attrinfo.list;
	var list = "<li>";
	var ctview, jumpid;
	if(type == 'view')
	{
		list += "<a href='#'>视图【" + attrinfo.view_id + "】已绑定的监控点列表";
		ctview = '#ds_ct_attr_show_list_' + attrinfo.view_id;
		jumpid = attrinfo.view_id;
	}
	else
	{
		list += "<a href='#'>机器【" + attrinfo.machine_id + "】有上报的监控点列表";
		ctview = '#dsm_ct_attr_show_list_' + attrinfo.machine_id;
		jumpid = attrinfo.machine_id;
	}
	list += "(共:" + attrinfo.count + "个)</a>";

	// 循环遍历，将相同监控点类型的监控点放一起
	for(var i=0; i < attrinfo.count && i < listattr.length; i++)
		listattr[i].pushed = false;
	for(var i=0; i < attrinfo.count && i < listattr.length; i++)
	{
		if(listattr[i].pushed)
			continue;

		if(type != 'site') {
			list += "<ul>"; 
			list += "<li><a href='#'>" + ds_attr_type_list[listattr[i].attr_type] + "</a>";
		}
		list += "<ul>";
		for(var j=i; j < attrinfo.count && j < listattr.length; j++)
		{
			if(listattr[j].pushed || listattr[j].attr_type != listattr[i].attr_type)
				continue;
			
			listattr[j].pushed = true;
			var attr_show_id = "attr_" + listattr[j].id + "_" + jumpid + type;
			list += "<li>";
			list += "<a onclick=\"dmtJumpToAttrPic('"+attr_show_id+"', '"+
				ctview+"', "+clickCallBack+")\" href='#' ctid='" + attr_show_id + "'>";

			list += listattr[j].id + "_" + listattr[j].name;
			if(listattr[j].global == 1) 
				list += "<font color='blue'>&nbsp;[全局]</font>";
		
			attr_list_arry.push(listattr[j].id);
			list += "</a>";
			list += "</li>";
		}
		list += "</ul>";
		list += "</li>";
		list += "</ul>";
	}

	list += "</li>";
	return list;
}

function dmtGetAttrIdList(attrinfo, attr_list_arry)
{
	var listattr = attrinfo.list;

	// 循环遍历，将相同监控点类型的监控点放一起
	for(var i=0; i < attrinfo.count && i < listattr.length; i++)
		listattr[i].pushed = false;
	for(var i=0; i < attrinfo.count && i < listattr.length; i++)
	{
		if(listattr[i].pushed)
			continue;
		for(var j=i; j < attrinfo.count && j < listattr.length; j++)
		{
			if(listattr[j].pushed || listattr[j].attr_type != listattr[i].attr_type)
				continue;
			
			listattr[j].pushed = true;
			attr_list_arry.push(listattr[j].id);
		}
	}
}

function dmtShowChangeValue(value)
{
	if(value >= 1073741824)
	{
		var f = value/1073741824;
		return Math.round(parseFloat(f)*100)/100 + 'G';
	}
	if(value >= 1048576)
	{
		var f = value/1048576;
		return Math.round(parseFloat(f)*100)/100 + 'M';
	}
	if(value >= 1024)
	{
		var f = value/1024;
		return Math.round(parseFloat(f)*100)/100 + 'K';
	}
	return value;
}

function dmtSetStrAttrInfoChartIpGeo(ct_id, attr_info, js, attr_val_list, showtype)
{
	var op = {
        backgroundColor: 'transparent',
		title: {
			x: 'left',
			subtext:'',
			top:5,
            subtextStyle: { 
                fontSize: 12,
            },
            textStyle:{
                fontSize: 14,
            },
			show:true,
			text:''
		},
		tooltip: {
			trigger:'item',
			confine:true,
			formatter: '{b}<br/>访问次数: {c}' 
		},
		visualMap: {
			min:0,
			max:10000,
			realtime:true,
			calculable: true,
			inRange: {
				color: ['lightskyblue', 'yellow', 'orangered']
			}
		},
		toolbox: {
			show: true,
			orient: 'horizontal',
			top: 'top',
            zlevel :999,
			right:5,
			feature: {
                myPluginShowAttrMach: {
                    show : false,
                    title : '查看该监控点在各机器的上报',
					icon:'image://'+g_siteInfo.doc_path+'images/color_swatch.png',
                    onclick : function() {}
                },
				myShowDetailData:{
					show:true,
					title:'图表概要信息',
					icon:'image://'+g_siteInfo.doc_path+'images/database_table.png',
					onclick:function(){}
				},
				dataView: {readOnly: false},
				restore: {},
				saveAsImage: { show:false }
			}
		},
		legend : {
			type:'scroll',
			orient: 'vertical',
			left:'left',
			top:'40',
			show:true
		},
		series: [
			{
				type:'map',
				mapType: 'china',
				label: {
					show:true
				},
				data:[],
                roam: true,
                zlevel: 999,
                backgroundColor: 'transparent',
                itemStyle: {
                    normal: {
                        areaColor: '#4c60ff',
                        borderColor: '#002097'
                    },
                    emphasis: {
                        areaColor: '#293fff'
                    }
                },
				geoIndex:0
			}
		]
	};

	$('#'+ct_id).css('height', g_dmtChartHeight);
    var warn_type_id = 0;
	if(showtype == 'view') {
		op.title.text = '视图ID【'+attr_val_list.view_id+'】'+'监控点：'+attr_info.name;
        warn_type_id = attr_val_list.view_id;
    }
    else if(showtype == 'pluginview') {
        op.title.text = '插件ID【'+attr_val_list.plugin_id+'】'+'监控点：'+attr_info.name;
        warn_type_id = attr_val_list.plugin_id;
    }
	else {
		op.title.text = '服务器ID【'+attr_val_list.machine_id+'】'+'监控点：'+attr_info.name;
        warn_type_id = attr_val_list.machine_id;
    }
	op.title.text += '-'+attr_info.id;

	if(js.str_count > 0) {
		var total = 0;
		for(var i=0; i < js.str_count; i++) {
			total += js.str_list[i].value;
		}
		op.visualMap.max = js.str_list[0].value+100;
		op.series[0].data = js.str_list;
	}
	else {
		op.series[0].data = [];
		op.title.subtextStyle.color = 'red';
		op.title.subtextStyle.fontWeight = 'bold';
		op.title.subtext = "暂无数据上报";
		op.title.x = 'center';
		op.title.top = 'center';
	}

    // 图表概要信息存入 warnInfo 中
	var warnInfo = new Object();
	dmtSetViewChartInfo(showtype, attr_val_list, js, attr_info, warnInfo);
	warnInfo.value_type = attr_info.value_type;
	warnInfo.attr_name = attr_info.name;
    warnInfo.str_count = js.str_count;
	op.toolbox.feature.myShowDetailData.onclick = function() {
		return dmtShowChartDetailInfo(
			showtype,
			warn_type_id,
			attr_info.id
		);
	};

	g_all_charts[ct_id] = echarts.init(document.getElementById(ct_id), g_dmtChartStyle);
	g_all_charts[ct_id].setOption(op);
	g_all_charts[ct_id].setwidth = g_dmtChartWidth;
	g_all_charts[ct_id].warnInfo = warnInfo;
}


function dmtSetStrAttrInfoChart(ct_id, attr_info, js, attr_val_list, showtype)
{
	var op = {
        backgroundColor: 'transparent',
		title: {
			x: 'left',
			subtext:'',
			top:5,
            subtextStyle: { 
                fontSize: 12,
            },
            textStyle:{
                fontSize: 14,
            },
			show:true,
			text:''
		},
		tooltip: {
			trigger:'item',
			confine:true,
			formatter: '上报字符串：{b}<br>上报值: {c} ({d}%)'
		},
		legend : {
			type:'scroll',
			orient: 'vertical',
			left:'left',
			top:'40',
			show:true
		},
		toolbox: {
			show: true,
			orient: 'horizontal',
			top: 'top',
            zlevel :999,
			right:5,
			feature: {
                myPluginShowAttrMach: {
                    show : false,
                    title : '查看该监控点各机器的上报',
                    icon : 'image://'+g_siteInfo.doc_path+'images/color_swatch.png',
                    onclick : function() {}
                },
				myShowDetailData:{
					show:true,
					title:'图表概要信息',
					icon:'image://'+g_siteInfo.doc_path+'images/database_table.png',
					onclick:function(){}
				},
				dataView: {readOnly: false},
				restore: {},
				saveAsImage: { show:false }
			}
		},
		series: [
			{
                backgroundColor: 'transparent',
				type:'pie',
				radius:'75%',
                zlevel: 999,
				center:['70%', '55%'],
				data:[],
				label: { 
					show: false,
	        		normal: {
            	        show: false,
            	        position: 'inside'
            	    },
            	    emphasis: {
            	        show: false,
            	    }
            	},
				itemStyle: {
					emphasis: {
						shadowBlur: 10,
						shadowOffsetX: 0,
						shadowColor: 'rgba(0, 0, 0, 0.5)'
					}
				}
			}
		]
	};

	$('#'+ct_id).css('height', g_dmtChartHeight);
    var warn_type_id = 0;
	if(showtype == 'view') {
		op.title.text = '视图ID【'+attr_val_list.view_id+'】'+'监控点：'+attr_info.name;
        warn_type_id = attr_val_list.view_id;
    }
    else if(showtype == 'pluginview') {
        op.title.text = '插件ID【'+attr_val_list.plugin_id+'】'+'监控点：'+attr_info.name;
        warn_type_id = attr_val_list.plugin_id;
    }
	else {
		op.title.text = '服务器ID【'+attr_val_list.machine_id+'】'+'监控点：'+attr_info.name;
        warn_type_id = attr_val_list.machine_id;
    }
	op.title.text += '-'+attr_info.id;
	
	if(js.str_count > 0) {
		op.series[0].data = js.str_list;
	}
	else {
		op.series[0].data = [];
		op.title.subtextStyle.color = 'red';
		op.title.subtextStyle.fontWeight = 'bold';
		op.title.subtext = "暂无数据上报";
		op.title.x = 'center';
		op.title.top = 'center';
	}

    // 图表概要信息存入 warnInfo 中
	var warnInfo = new Object();
	dmtSetViewChartInfo(showtype, attr_val_list, js, attr_info, warnInfo);
    warnInfo.str_count = js.str_count;
	warnInfo.show_class = attr_info.show_class;
	warnInfo.attr_name = attr_info.name;
	op.toolbox.feature.myShowDetailData.onclick = function() {
		return dmtShowChartDetailInfo(
			showtype,
			warn_type_id,
			attr_info.id
		);
	};

	g_all_charts[ct_id] = echarts.init(document.getElementById(ct_id), g_dmtChartStyle);
	g_all_charts[ct_id].setOption(op);
	g_all_charts[ct_id].warnInfo = warnInfo;
	g_all_charts[ct_id].setwidth = g_dmtChartWidth;
}

function dmtGetXAxisTimeInfo(dateStart, count_day, attr_info, max)
{
	var time_info = [];
	for(var i=0; i < attr_info.static_idx_max*count_day; i++) {
        if(typeof max == 'number' && i >= max)
            break;
		time_info.push(dateStart+i*attr_info.static_time*60*1000);
	}
	return time_info;
}

function dmtGetXAxisDayTimeInfo(dateStart, count_day)
{
	var time_info = [];
	for(var i=0; i < count_day; i++) {
		time_info.push(dateStart+i*24*60*60*1000);
	}
	return time_info;
}

// 根据每天的统计周期数据，计算每天的监控点数据增长
// dstr - 为每天的统计周期数据
// attr_info - 监控点
// day_time_info - 为每天的日期
function dmtGetYAxisDayAddData(day_time_info, attr_info, dstr, cur)
{
	var e_data_y = [];
	var attr_data = dstr.split(",");
    if(day_time_info.length <= 0 || attr_data.length <= 0)
        return e_data_y;

	// 跳过统计周期开始的 0 数据
	var data_start = 0;

    // 计算第一天的增长
    // 获取第一个统计周期数据
    var first_idx_val = 0;
    var t = 0;
    for(; t < attr_info.static_idx_max && t < attr_data.length; t++) {
        if(attr_data[t] != "-" && !isNaN(attr_data[t])) {
            first_idx_val = Number(attr_data[t]);
            break;
        }
    }

    // 获取最后一个统计周期数据
    var last_idx_val = first_idx_val;
    for(var k=attr_info.static_idx_max-1; k > t && k < attr_data.length; k--) {
        if(attr_data[k] != "-" && !isNaN(attr_data[k])) {
            last_idx_val = Number(attr_data[k]);
            break;
        }
    }

    var objd = new Object;
	objd.value = new Array(day_time_info[0], last_idx_val-first_idx_val);
	e_data_y.push(objd);
    if(day_time_info.length <= 1)
        return e_data_y;
 
    // 计算非第一天非最后一天的增长(前一天的最后一个统计周期, 用第一天的初始化)
    var last_pre_val = last_idx_val;
	for(var j=0, iDays = 1; iDays < day_time_info.length-1; iDays++) 
    {
        // 获取当天最后一个统计周期数据(先用前一天的最后统计初始化，规避无上报的问题)
        last_idx_val = last_pre_val;
        var j = (iDays+1)*attr_info.static_idx_max-1;
        for(var k=attr_info.static_idx_max; k > 0 && j >= 0 && j < attr_data.length; k--, j--) {
            if(attr_data[j] != "-" && !isNaN(attr_data[j])) {
                last_idx_val = Number(attr_data[j]);
                break;
            }
        }

        // 计算增长
        var iAdd = 0;
        if(last_idx_val != 0 && last_idx_val != last_pre_val) {
			if(data_start == 0 && last_pre_val == 0) 
				data_start = 1;
			else 
            	iAdd = last_idx_val - last_pre_val;
            last_pre_val = last_idx_val;
        }

	    var objd = new Object;
		objd.value = new Array(day_time_info[iDays], iAdd);
		e_data_y.push(objd);
	}

	// 计算最后一天的增长
	var iAdd = 0;
	var last_day_idx_start = (day_time_info.length-1)*attr_info.static_idx_max;
	
	// 2,4,5,8 这几种类型的数据不需要统计周期结束即可使用，因此可以用 cur 做来做差值计算
	if((attr_info.data_type == 2 || attr_info.data_type == 4 || attr_info.data_type == 5
	    || attr_info.data_type == 8) && cur != 0 && attr_info.length > last_day_idx_start)
	{
	    if(last_pre_val != cur)
	        iAdd = cur - last_pre_val;
	}
	else if(attr_data.length >= last_day_idx_start)
	{
	    for(var j=attr_data.length-1; j >= last_day_idx_start; j--) {
	        if(attr_data[j] != "-" && !isNaN(attr_data[j])) {
	            last_idx_val = Number(attr_data[j]);
	            break;
	        }
	    }
	    if(last_idx_val != 0 && last_idx_val != last_pre_val) {
	        if(data_start != 0 || last_pre_val != 0)
	            iAdd = last_idx_val - last_pre_val;
	    }
	}

    var objd = new Object;
    objd.value = new Array(day_time_info[iDays], iAdd);
    e_data_y.push(objd);
	return e_data_y;
}

function dmtGetStartXAxis(sdata, last_count)
{
    if(sdata.length == 0)
        return 0;
    if(sdata.length >= last_count)
        return 100-dmtMathtrunc(last_count*100/sdata.length);
    return 0;
}

function dmtGetYAxisData(time_info, dstr, use_all, max)
{
    if(dstr == "0")
        return [];

	var e_data_y = [];
	var attr_data = dstr.split(",");
	for(var j=0; j < attr_data.length; j++) {
        if(typeof max == 'number' && j >= max)
            break;
		if(typeof use_all != 'undefined' || (attr_data[j] != "-" && !isNaN(attr_data[j]))) {
            var objd = new Object;
            if(isNaN(attr_data[j]))
                objd.value = new Array(time_info[j], 0);
            else
			    objd.value = new Array(time_info[j], attr_data[j]);
			e_data_y.push(objd);
		}
	}
	return e_data_y;
}

function dmtGetUsePerPieOption(options) 
{
	var op = $.extend({ c_items:1, c_sublink:'' }, options);

	var subtext_color = 'blue';
	var text_color = '#044';
	if(op.c_colors.length > 4)
		subtext_color = op.c_colors[4];
	if(op.c_items > 1) 
		text_color = 'blue';
	var pie_option = {
        backgroundColor: 'transparent',
	    title: {
			id:op.c_name,
	        text: op.c_text,
			triggerEvent: true,
			subtext: op.c_subtext,
	        left: 'center',
	        top: 'center',
	        textStyle: {
	        //    color: text_color,
	            fontSize: 14,
	            fontFamily: 'PingFangSC-Regular'
	        },
        	subtextStyle: {
        	//   color: subtext_color,
        	    fontSize: op.c_items > 1 ? 14 : 18,
        	    fontFamily: 'PingFangSC-Regular',
				fontWeight: 'bold',
        	    top: 'center'
        	},
			itemGap: -1,
	    },
	    series: [{
	        name: op.c_name,
	        type: 'pie',
	        clockWise: true,
	        radius: ['80%', '95%'],
	        itemStyle: {
	            normal: {
	                label: {
	                    show: false
	                },
	                labelLine: {
	                    show: false
	                }
	            }
	        },
	        hoverAnimation: false,
	        data: [{
	            value: op.c_val,
	            name: 'completed',
	            itemStyle: {
	                normal: {
	                    borderWidth: 8,
	                    borderColor: { 
	                        colorStops: [{
	                            offset: 0,
	                            color: op.c_colors[0] || op.c_colors[1]
	                        }, {
	                            offset: 1,
	                            color: op.c_colors[2] || op.c_colors[3]
	                        }]
	                    },
	                    color: {
	                        colorStops: [{
	                            offset: 0,
	                            color: op.c_colors[0] || op.c_colors[1]
	                        }, {
	                            offset: 1,
	                            color: op.c_colors[2] || op.c_colors[3]
	                        }]
	                    },
	                    label: {
	                        show: false
	                    },
	                    labelLine: {
	                        show: false
	                    }
	                }
	            }
	        }, {
	            name: 'gap',
	            value: 100 - op.c_val,
	            itemStyle: {
	                normal: {
	                    label: {
	                        show: false
	                    },
	                    labelLine: {
	                        show: false
	                    },
	                    color: 'rgba(0, 0, 0, 0)',
	                    borderColor: 'rgba(0, 0, 0, 0)',
	                    borderWidth: 0
	                }
	            }
	        }]
	    }]
	};

	if(op.c_sublink != '') {
		pie_option.title.sublink = op.c_sublink;
		pie_option.title.subtarget = 'self';
	}
	return pie_option;
}

function dmtShowPluginAttrInfo(js, ct_div)
{
	var op = {
        backgroundColor: 'transparent',
		legend:{
			show:false,
			bottom:10
		},
		title:{
			text:'',
			x:'left',
            left:10,
            subtext:'',
            top:5,
            subtextStyle: { 
                fontSize: 12,
                color:'red'
            },
            textStyle:{
                fontSize: 14
            },
			show:true
		},
		useUTC:true,
		toolbox: {
			show:true,
			orient: 'horizontal',
			top: 'top',
            zlevel :999,
			right:5,
			feature: {
				restore: { 
					show:true,
					title: '还原图表显示'
				},
				dataZoom:{
					show:true,
					xAxisIndex:0,
					yAxisIndex:false
				},
				saveAsImage: { show: false }
			}
		},
	    tooltip: {
            formatter: null,
			trigger: 'axis'
		},
		grid: {
            backgroundColor: 'transparent',
			tooltip: {
				show:true,
				trigger:'axis'
			},
            show:false,
			top:50,
            left:60,
            right:30,
			bottom:50
		},
	    xAxis: {
			show:true,
			type: 'time',
            splitLine: {
                show: false
            },
            axisPointer: {
                label: {
                    formatter: function (params) {
                        return '上报时间：' + dmtGetDateStr(params.value, true);
                    }
                }
            }
	    },
		yAxis: {
            splitLine: {
                show: false
            },
			splitArea: {
				show: false 
			},
			axisLabel:{
				formatter: null
			},
			show:true,
			type: 'value'
		},
		dataZoom: [
			{
				type:'inside',
				xAxisIndex: 0
			}
		],
		series: [
			{
				name:'',
				showSymbol:false,
				cursor:'pointer',
				smooth: true,
				smoothMonotone:'x',
				type:'line',
                itemStyle : {  
                    normal : {  
                       color:'#009895'
                    },
                    lineStyle:{
                        normal:{
                        color:'#009895',
                        opacity:1
                        }
                    }
                },
                sampling : 'average',
                areaStyle: null,
                stack: null,
				data:[]
			},
			{
				name:'',
				showSymbol:false,
				cursor:'pointer',
				smooth: true,
				smoothMonotone:'x',
				type:'line',
                itemStyle : {  
                    normal : {  
                        color:'#F3891B'
                    },
                    lineStyle:{
                        normal:{
                        color:'#F3891B',
                        opacity:1
                            }
                    }
                },  
                sampling : 'average',
                areaStyle: null,
                stack: null,
				data:[]
			},
			{
				name:'',
				showSymbol:false,
				cursor:'pointer',
				smooth: true,
				smoothMonotone:'x',
				type:'line',
                itemStyle : {  
                    normal : {  
                        color:'#006AD4'
                    },
                    lineStyle:{
                        normal:{
                        color:'#F3891B',
                        opacity:1
                            }
                    }
                },
                sampling : 'average',
                areaStyle: null,
                stack: null,
				data:[]
			}
		]
	};

	if(typeof js.value_list_str == "undefined" || js.attr_list == 'undefined' || js.attr_list.count <= 0)
		return;

	var dateStart = js.date_time_start_utc*1000 - new Date().getTimezoneOffset()*60*1000;
	var count_day = 1;
	if(js.type == 3)
		count_day = 7;

	var attr_info = js.attr_list.list[0];
    var time_info = dmtGetXAxisTimeInfo(dateStart, count_day, attr_info, js.max_x);
    
    // 汇总图
    if(js.max != 0) {
        if(js.type == 1)
            op.series[0].data = dmtGetYAxisData(time_info, js.value_list_str, true, js.max_x);
        else
            op.series[0].data = dmtGetYAxisData(time_info, js.value_list_str);
        if(js.type == 1)
		{
			op.series[0].name = '今日 [' + js.date_time_cur + ']';
			op.series[1].name = '昨日 [' + js.date_time_yst + ']';
			op.series[1].data = dmtGetYAxisData(time_info, js.value_list_yst_str, true, js.max_x);
			op.series[2].name = '上周同日 [' + js.date_time_wkd + ']';
			op.series[2].data = dmtGetYAxisData(time_info, js.value_list_lwk_str, true, js.max_x);
			op.legend = {
				show:true,
				bottom:10
			};
			op.grid.bottom = 80;
		}

        var attr_show_id = "attr_0_" + js.id +'pluginattr';
        if($('#'+attr_show_id).length > 0)
            $('#'+attr_show_id).html("");
	    var attr_show_container_str = '<div style="width:' + g_dmtChartWidth + 'px; ';
	    attr_show_container_str += ' height:' + g_dmtChartHeight + 'px; ';
	    attr_show_container_str += ' float:left; margin:' + g_dmtChartMargin + 'px;" ';
	    attr_show_container_str += ' type="pluginattr" ';
	    attr_show_container_str += ' id="' + attr_show_id+ '" ';
	    attr_show_container_str += ' class="chart-DES-PWE" ';
	    attr_show_container_str += ' type_id="' + js.id + '" >';
	    attr_show_container_str += '</div>';
	    $(ct_div).append( $(attr_show_container_str) );
        if(typeof(g_all_charts[attr_show_id]) != 'undefined')
            g_all_charts[attr_show_id].dispose();
	    op.title.text = " 汇总图 - " + attr_info.name;
        g_all_charts[attr_show_id] = echarts.init(document.getElementById(attr_show_id), g_dmtChartStyle);
        dmtSetChartOptionByAttrInfo(attr_info, op, {day_type:js.type});
        g_all_charts[attr_show_id].setOption(op);
    }
	
    // 各机器的上报图
	var bHasData = true;
    var mach_attr_vals = js.each_mach.list;
	for(var i=0; i < mach_attr_vals.length; i++)
	{
		// 确保图表 ID 全局唯一
		var attr_show_id = "attr_" + mach_attr_vals[i].id + '_' + js.id + "pluginattr";
		if($('#'+attr_show_id).length > 0)
		{
			dmtJsBugReport('dmt.comm.js', 'dmtShowPluginAttrInfo', 'bug:'+attr_show_id);
			$('#'+attr_show_id).html("");
		}

		var attr_show_container_str = '<div style="width:' + g_dmtChartWidth + 'px; ';
		attr_show_container_str += ' height:' + g_dmtChartHeight + 'px; ';
		attr_show_container_str += ' float:left; margin:' + g_dmtChartMargin + 'px;" ';
		attr_show_container_str += ' type="pluginattr" ';
		attr_show_container_str += ' id="' + attr_show_id+ '" ';
		attr_show_container_str += ' class="chart-DES-PWE" ';
		attr_show_container_str += ' type_id="' + js.id + '" >';
		attr_show_container_str += '</div>';

		$(ct_div).append( $(attr_show_container_str) );
		if(typeof(g_all_charts[attr_show_id]) != "undefined") 
			g_all_charts[attr_show_id].dispose();
        var time_info = dmtGetXAxisTimeInfo(dateStart, count_day, attr_info);
		if(typeof mach_attr_vals[i].value_list_str != 'undefined' && mach_attr_vals[i].max > 0) {
            if(js.type == 1)
		    	op.series[0].data = dmtGetYAxisData(time_info, mach_attr_vals[i].value_list_str, true, js.max_x);
            else
		    	op.series[0].data = dmtGetYAxisData(time_info, mach_attr_vals[i].value_list_str);
			bHasData = true;
		}
		else  {
			op.series[0].data = [];
			bHasData = false;
		}

		if(js.type == 1)
		{
			op.series[0].name = '今日 [' + js.date_time_cur + ']';

			if(typeof js.each_mach_yst.list[i].value_list_str != "undefined")
				op.series[1].data = dmtGetYAxisData(time_info, js.each_mach_yst.list[i].value_list_str, true, js.max_x);
			else
				op.series[1].data = [];
			op.series[1].name = '昨日 [' + js.date_time_yst + ']';

			if(typeof js.each_mach_lwk.list[i].value_list_str != "undefined")
				op.series[2].data = dmtGetYAxisData(time_info, js.each_mach_lwk.list[i].value_list_str, true, js.max_x);
			else
				op.series[2].data = [];
			op.series[2].name = '上周同日 [' + js.date_time_wkd + ']';
            if(js.each_mach_yst.list[i].value_list_str != "0" || js.each_mach_lwk.list[i].value_list_str != "0")
				bHasData = true;
		}

		// 5 -- 历史积累监控点类型
		if(attr_info.data_type == 5)
			op.yAxis.min = mach_attr_vals[i].min;
		else
			op.yAxis.min = 0;
        if(mach_attr_vals[i].name != mach_attr_vals[i].ip1)
    		op.title.text = '上报机器【' + mach_attr_vals[i].id + '】'+ mach_attr_vals[i].name + '('+mach_attr_vals[i].ip1+')';
        else
    		op.title.text = '上报机器【' + mach_attr_vals[i].id + '】'+ mach_attr_vals[i].name; 

		if(!bHasData) {
            op.title.textStyle.fontSize = 16;
            op.title.subtextStyle.fontSize = 14;
            if(mach_attr_vals[i].cur > 0) {
                var subtitle = " 当前时间：" + attr_val_list.date_time + ', ';
                subtitle += " 当前统计周期已上报：" + dmtShowChangeValue(mach_attr_vals[i].cur) + ', ';
                subtitle += " 统计周期：" + dmtShowHumanStaticTime(attr_info.static_time) + "\n\n";
                op.title.subtext = subtitle;
            }
            else
                op.title.subtext = '暂无数据上报';
            op.title.link = '';
			op.title.top = 'center';
			op.title.left = 'center';
			op.xAxis.show = false;
			op.yAxis.show = false;
			op.toolbox.feature.dataZoom.show = false;
		}
		else {
            op.title.textStyle.fontSize = 14;
            op.title.link = '';
            op.title.subtextStyle.fontSize = 12;
            op.title.subtext = '';
			op.title.top = 5;
			op.title.left = 10;
			op.xAxis.show = true;
			op.yAxis.show = true;
			op.toolbox.feature.dataZoom.show = true;
		}

		g_all_charts[attr_show_id] = echarts.init(document.getElementById(attr_show_id), g_dmtChartStyle);
        dmtSetChartOptionByAttrInfo(attr_info, op, {day_type:js.type});
		g_all_charts[attr_show_id].setOption(op);
		g_all_charts[attr_show_id].setwidth = g_dmtChartWidth;
	}
}

function dmtShowAttrLineAndPie(js, ct_div, options, op_series_options, op_pie_options)
{
    var op_series = $.extend({
        stack : 'line_pie_'+js.id,
        showAllSymbol : 'auto',
        symbol : 'emptyCircle',
        cursor:'pointer',
        smoothMonotone:'x',
		smooth: true,
		type:'line',
        sampling : 'average'
    }, op_series_options);

    var op_pie = $.extend({
		type:'pie',
        id:'line_pie_id',
		radius:'30%',
        top :5,
		center:['38%', '30%'],
		data:[], 
        tooltip: {
            show : true,
            confine:true,
            trigger:'item',
            formatter : '{b}: {c}'
        }
    }, op_pie_options);

	var op = $.extend({
        backgroundColor: 'transparent',
		legend:{
			show:true,
            orient : 'orient',
            top : 30,
            right : 10
		},
	    tooltip: {
            formatter : function(p, t, ck) {
                var h = p[0].axisValueLabel;
                for(var i=0; i < p.length; i++) {
                    h += '<br />' + p[i].seriesName + ': ' + p[i].value[1] + '%';
                }
                return h;
            },
			trigger: 'axis'
		},
		title:{
			text:js.title,
            subtext:'',
			x:'left',
            left:10,
            top:5,
            subtextStyle: { 
                fontSize: 12
            },
            textStyle:{
                fontSize: 14
            },
			show:true
		},
		useUTC:true,
		grid: {
            backgroundColor: 'transparent',
			tooltip: {
				show:true,
				trigger:'axis'
			},
            show:false,
			top:'55%',
            left:60,
            right:30,
			bottom:50
		},
	    xAxis: {
			show:true,
			type: 'time',
            splitLine: {
                show: false
            },
            gridIndex :0,
            axisPointer: {
                label: {
                    formatter: function (params) {
                        return '时间：' + dmtGetDateStr(params.value, true);
                    }
                }
            }
	    },
        color: g_dmtChartColor,
		yAxis: {
            splitLine: {
                show: false
            },
			splitArea: {
				show: false 
			},
            axisLabel : { 
                formatter : '{value}%'
            },
            type :'value',
            gridIndex: 0
		},
		dataZoom: [
			{
				type:'inside',
                start : 0,
                end : 100,
				xAxisIndex: 0
			}
		],
		series: []
	}, options);

	if(typeof js.attr_val_list == "undefined" || js.attr_list == 'undefined' || js.attr_list.length <= 0)
		return;

	var dateStart = js.date_time_start_utc*1000 - new Date().getTimezoneOffset()*60*1000;
	var attr_info = js.attr_list[0];

    var days = 1;
    if(typeof js.day_count == 'number')
        days = js.day_count;
    var time_info = dmtGetXAxisTimeInfo(dateStart, days, attr_info, js.attr_val_list[0].max_x);

	var attr_show_id = "line_pie_attr_" + js.attr_val_list[0].id + '_' + js.id + js.showtype;
	if($('#'+attr_show_id).length > 0)
	{
		dmtJsBugReport('dmt.comm.js', 'dmtShowAttrLineAndPie', 'bug:'+attr_show_id);
		$('#'+attr_show_id).html("");
	}

    var chartHeight = g_dmtChartHeight*3/2;
    if(typeof js.chartHeight == 'number')
        chartHeight = js.chartHeight;
	var attr_show_container_str = '<div style="width:' + g_dmtChartWidth + 'px; ';
	attr_show_container_str += ' height:' + chartHeight + 'px; ';
	attr_show_container_str += ' float:left; margin:' + g_dmtChartMargin + 'px;" ';
	attr_show_container_str += ' type="pluginattr" ';
	attr_show_container_str += ' id="' + attr_show_id+ '" ';
	attr_show_container_str += ' class="chart-DES-PWE" ';
	attr_show_container_str += ' type_id="' + js.id + '" >';
	attr_show_container_str += '</div>';
	$(ct_div).prepend( $(attr_show_container_str) );
	if(typeof(g_all_charts[attr_show_id]) != "undefined") 
		g_all_charts[attr_show_id].dispose();

    var max_x = 0;
	for(var i=0; i < js.attr_val_list.length; i++)
    {
        op.series.push(dmtCloneObj(op_series));
        op.series[i].name = js.attr_list[i].c_short_name;
        op.series[i].data = dmtGetYAxisData(time_info, js.attr_val_list[i].value_list_str, true);
        if(js.attr_val_list[i].max_x > max_x)
            max_x = js.attr_val_list[i].max_x;
    }

    var showCountStatics = 30;
    if(typeof js.show_count_statics == 'number')
        showCountStatics = js.show_count_statics;
    if(max_x != 0) {
        op.dataZoom[0].start = dmtMathtrunc(100-100*showCountStatics/max_x);
        if(op.dataZoom[0].start < 0)
            op.dataZoom[0].start = 0;
    }

    if(typeof js.total_value == 'number' && typeof js.other_value_name == 'string' && js.draw_other_line)
    {
        var otherData = [];
        for(var j=0; j < op.series[0].data.length; j++) {
            var objd = new Object;
            var t = 0;
            for(var i=0; i < op.series.length; i++) {
                t += Number(op.series[i].data[j].value[1]);
            }
            if(js.total_value - t > 0)
                objd.value = new Array(op.series[0].data[j].value[0], js.total_value - t);
            else
                objd.value = new Array(op.series[0].data[j].value[0], 0);
            otherData.push(objd);
        }
        op.series.push(dmtCloneObj(op_series));
        op.series[i].name = js.other_value_name;
        op.series[i].data = otherData;
    }

    // 初始化饼图 series
    op.series.push(op_pie);

    var piefmtstr = '{b}：{d}%';
    if(typeof js.is_value_percent != 'undefined' && js.is_value_percent)
        piefmtstr = '{b}：{c}%';

	g_all_charts[attr_show_id] = echarts.init(document.getElementById(attr_show_id), g_dmtChartStyle);
	g_all_charts[attr_show_id].setOption(op);
	g_all_charts[attr_show_id].setwidth = g_dmtChartWidth;

    // 饼图信息设置
    var fpie = function(idx) {
        var upPieData = [];
        var cur_total = 0;
        for(var i=0; i < op.series.length-1; i++) {
            if(Number(op.series[i].data[idx].value[1]) > 0) {
                upPieData.push( {name: op.series[i].name, value: op.series[i].data[idx].value[1] });
                cur_total += Number(op.series[i].data[idx].value[1]);
            }
        }

        if(typeof js.total_value == 'number' && typeof js.other_value_name == 'string' && !js.draw_other_line) {
            if(js.total_value - cur_total > 0) {
                upPieData.push({name: js.other_value_name, value: js.total_value-cur_total});
            }
        }

        g_all_charts[attr_show_id].setOption({
            title : {
                subtext : '时间：'+ dmtGetDateStr(op.series[0].data[idx].value[0])
            },
            series: {
                id: 'line_pie_id',
                label : { show: true, formatter: piefmtstr },
                data: upPieData
            }
        });
    };
    fpie(op.series[0].data.length-1);
    g_all_charts[attr_show_id].on('updateAxisPointer', function (event) {
        if (typeof event.dataIndex == 'number') {
            fpie(event.dataIndex);
        }
    });
}

function dmtCloneObj(obj) {
    var tempObj = {};
    if (obj instanceof Array) {
        tempObj = [];
    }
    for (var prop in obj) {
        if (typeof prop === 'Object') {
            cloneObj(prop);
        }
        tempObj[prop] = obj[prop];
    }
    return tempObj;
}

function dmtShowAttr2Line(js, ct_div, options, op_series_1_options, op_series_2_options)
{
    var op_series_1 = $.extend({
        stack : '',
        showAllSymbol : 'auto',
        symbol : 'emptyCircle',
        cursor:'pointer',
        smoothMonotone:'x',
		smooth: true,
		type:'line',
        xAxisIndex: 0,
        yAxisIndex: 0,
        sampling : 'average'
    }, op_series_1_options);

    var op_series_2 = $.extend({
        stack : 'two_line_2_'+js.id,
        showAllSymbol : 'auto',
        symbol : 'emptyCircle',
        cursor:'pointer',
        smoothMonotone:'x',
		smooth: true,
        xAxisIndex: 1,
        yAxisIndex: 1,
        areaStyle : {},
		type:'line',
        sampling : 'average'
    }, op_series_2_options);

	var op = $.extend({
        backgroundColor: 'transparent',
		legend: {
			show:true,
            orient : 'horizontal',
            top : '50%',
            left: 10
        },
	    tooltip: {
			trigger: 'axis'
		},
		title:[ {
			text:js.title_1,
            subtext:'',
			x:'left',
            left:10,
            top:5,
            subtextStyle: { 
                fontSize: 12
            },
            textStyle:{
                fontSize: 14
            },
			show:true
		  }, {
			text:js.title_2,
            subtext:'',
            right:30,
            top:'55%',
            subtextStyle: { 
                fontSize: 12
            },
            textStyle:{
                fontSize: 14
            },
			show:false
          } 
        ],
		useUTC:true,
		grid: [ {
            backgroundColor: 'transparent',
			tooltip: {
				show:true,
				trigger:'axis'
			},
			bottom:'58%',
            left:60,
            right:25,
		  }, {
            backgroundColor: 'transparent',
			tooltip: {
				show:true,
				trigger:'axis'
			},
			top:'62%',
            height:'30%',
            left:60,
            right:25,
          }
        ],
	    xAxis: [ {
			show:true,
			type: 'time',
            splitLine: {
                show: false
            },
            gridIndex : 0,
            axisPointer: {
                label: {
                    formatter: function (params) {
                        return '时间：' + dmtGetDateStr(params.value, true);
                    }
                }
            }
	      }, {
			show:true,
			type: 'time',
            splitLine: {
                show: false
            },
            gridIndex :1,
            axisPointer: {
                label: {
                    formatter: function (params) {
                        return '时间：' + dmtGetDateStr(params.value, true);
                    }
                }
            }
          }
        ],
        color: g_dmtChartColor,
		yAxis: [ {
            splitLine: {
                show: false
            },
			splitArea: {
				show: false 
			},
            axisLabel : { 
                formatter : function(v, idx) { return dmtGetHumanReadFixed2(v); }
            },
            gridIndex: 0,
            type :'value'
		  }, {
            splitLine: {
                show: false
            },
			splitArea: {
				show: false 
			},
            axisLabel : { 
                formatter : function(v, idx) { return dmtGetHumanReadFixed2(v); }
            },
            type :'value',
            gridIndex: 1
          }
        ],
		dataZoom: [{
			type:'inside',
            start : 0,
            end : 100,
			xAxisIndex: 0
		  }, {
			type:'inside',
            start : 0,
            end : 100,
			xAxisIndex: 1
          }
		],
		series: []
	}, options);

	if(js.attr_list_1.length <= 0 && js.attr_list_2.length <= 0)
		return;

	var dateStart = js.date_time_start_utc*1000 - new Date().getTimezoneOffset()*60*1000;
	var attr_info = null;
    if(js.attr_list_1.length > 0)
        attr_info = js.attr_list_1[0];
    else
        attr_info = js.attr_list_1[0];

    var days = 1;
    if(typeof js.day_count == 'number')
        days = js.day_count;

    var max_x = 0;
    if(js.attr_val_list_1.length > 0)
        max_x = js.attr_val_list_1[0].max_x;
    else
        max_x = js.attr_val_list_2[0].max_x;

    var time_info = dmtGetXAxisTimeInfo(dateStart, days, attr_info, max_x);

	// 确保图表 ID 全局唯一
	var attr_show_id = "line2_attr_" + attr_info.id + '_' + js.id + js.showtype;
	if($('#'+attr_show_id).length > 0)
	{
		dmtJsBugReport('dmt.comm.js', 'dmtShowAttrLineAndPie', 'bug:'+attr_show_id);
		$('#'+attr_show_id).html("");
	}

    var chartHeight = g_dmtChartHeight*3/2;
    if(typeof js.chartHeight == 'number')
        chartHeight = js.chartHeight;

	var attr_show_container_str = '<div style="width:' + g_dmtChartWidth + 'px; ';
	attr_show_container_str += ' height:' + chartHeight + 'px; ';
	attr_show_container_str += ' float:left; margin:' + g_dmtChartMargin + 'px;" ';
	attr_show_container_str += ' type="pluginattr" ';
	attr_show_container_str += ' id="' + attr_show_id+ '" ';
	attr_show_container_str += ' class="chart-DES-PWE" ';
	attr_show_container_str += ' type_id="' + js.id + '" >';
	attr_show_container_str += '</div>';
	$(ct_div).prepend( $(attr_show_container_str) );
	if(typeof(g_all_charts[attr_show_id]) != "undefined") 
		g_all_charts[attr_show_id].dispose();

    op.title[0].subtext = '日期：' + js.date_time_cur;

    // line chart 1
    var i = 0;
    max_x = 0;
	for(i=0; i < js.attr_val_list_1.length; i++)
    {
        op.series.push(dmtCloneObj(op_series_1));
        op.series[i].name = js.attr_list_1[i].name;
        op.series[i].data = dmtGetYAxisData(time_info, js.attr_val_list_1[i].value_list_str, true);
        if(js.attr_val_list_1[i].max_x > max_x)
            max_x = js.attr_val_list_1[i].max_x;
    }

    // 默认只显示最近 30 个统计周期的数据(用户可拖到图表更多)
    var showCountStatics = 30;
    if(typeof js.show_count_statics == 'number')
        showCountStatics = js.show_count_statics;
    if(max_x != 0) {
        op.dataZoom[0].start = dmtMathtrunc(100-100*showCountStatics/max_x);
        if(op.dataZoom[0].start < 0)
            op.dataZoom[0].start = 0;
    }

    // line chart 2
    max_x = 0;
	for(var k=0; k < js.attr_val_list_2.length; k++, i++)
    {
        op.series.push(dmtCloneObj(op_series_2));
        op.series[i].name = js.attr_list_2[k].name;
        op.series[i].data = dmtGetYAxisData(time_info, js.attr_val_list_2[k].value_list_str, true);
        if(js.attr_val_list_2[k].max_x > max_x)
            max_x = js.attr_val_list_2[k].max_x;
    }

    // 默认只显示最近 30 个统计周期的数据(用户可拖到图表更多)
    var showCountStatics = 30;
    if(typeof js.show_count_statics == 'number')
        showCountStatics = js.show_count_statics;
    if(max_x != 0) {
        op.dataZoom[1].start = dmtMathtrunc(100-100*showCountStatics/max_x);
        if(op.dataZoom[1].start < 0)
            op.dataZoom[1].start = 0;
    }

	g_all_charts[attr_show_id] = echarts.init(document.getElementById(attr_show_id), g_dmtChartStyle);
	g_all_charts[attr_show_id].setOption(op);
	g_all_charts[attr_show_id].setwidth = g_dmtChartWidth;
}

function dmtSetTreeChartData(js)
{
    switch(js.xrk_type) {
        case 1:
            js.symbol = 'image://'+g_siteInfo.doc_path+'images/xrk_server.png';
            js.symbolSize = [35,40];
            break;

        case 2:
            js.symbol = 'image://'+g_siteInfo.doc_path+'images/xrk_conn.png';
            js.symbolSize = [30,25];
            js.label = { fontWeight:'bold', distance:10};
            js.name += '\n ' + js.xrk_ip_geo;
            break;

        case 3:
            js.symbol = 'image://'+g_siteInfo.doc_path+'images/xrk_gw_conn.png';
            js.symbolSize = [30,25];
            break;

        case 4:
            js.symbol = 'image://'+g_siteInfo.doc_path+'images/xrk_gw.png';
            js.symbolSize = [30,25];
            break;

        default:
            js.symbol = 'image://'+g_siteInfo.doc_path+'images/clt_server.png';
            js.symbolSize = [20,25];
            if(js.xrk_req_status == 3 || js.xrk_req_status == 1) {
                js.label = { color: 'red', fontWeight:'bold', distance:8};
                js.name = js.name + '\n ' + '无心跳';
            }
            else if(js.xrk_req_status == 2) {
                js.label = { color: 'blue', fontWeight:'bold', distance:8};
                js.name += '\n ' + js.xrk_response_ms + 'ms / ' + js.xrk_jumps + ' 跳';
            }
            else {
                js.label = { color: 'white', fontWeight:'bold', distance:8};
                js.name += '\n ' + js.xrk_response_ms + 'ms / ' + js.xrk_jumps + ' 跳';
            }
            break;
    }

    if(typeof js.children == 'undefined')
        return;

    for(var i=0; i < js.children.length; i++)
        dmtSetTreeChartData(js.children[i]);
}

function dmtGetMachineReqStatus(s)
{
    switch(s) {
        case 3:
            return '<span class="xrk_show_txt_hs">长时间离线</span>';
        case 2:
            return '<span class="xrk_text_bl">数据上报中</span>';
        case 1:
            return '<span class="xrk_show_txt_hs">无心跳</span>';
        case 0:
            return '<span class="xrk_text_bl2">心跳正常</span>';
        default:
            return '<span color="xrk_show_txt_hs">未知状态:'+s+'</span>';
    }
}

function dmtGetTopData(data, w, h)
{
    var i=0;
    for(i=0;  i < data.length; i++) 
    {
        if(data[i].x == 'null')
            data[i].x = null;
        else
            data[i].x = data[i].x*w/100;
        if(data[i].y == 'null')
            data[i].y = null;
        else
            data[i].y = data[i].y*h/100;
        if(data[i].itemStyle == 'null')
            data[i].itemStyle = null;
    }
    return data;
}

function dmtShowTreeChart(attr_info, ct_id)
{
	var op = {
        backgroundColor: 'transparent',
        tooltip : {
            show: true,
            formatter : function(p) {
                var s = '';
                var pm = p.name;
                if(p.name.indexOf('\n') != -1)
                    pm = p.name.substring(0, p.name.indexOf('\n'));
                switch(p.data.xrk_type) {
                    case 1:
                        s += '云框架系统服务端<br />';
                        break;
                    case 2:
                        s += '接入服务器<br />接入地址：' + pm;
                        s += '<br />IP 归属地：' + p.data.xrk_ip_geo;
                        s += '<br />运营商：' + p.data.xrk_ip_owner;
                        break;
                    case 3:
                        s += '中间路由器群...';
                        break;
                    case 4:
                        s += '节点网关<br />网关地址：' + pm + '<br />节点数：'+p.data.children.length;
                        s += '<br />IP 归属地：' + p.data.xrk_ip_geo;
                        s += '<br />运营商：' + p.data.xrk_ip_owner;
                        break;
                    case 5:
                        s += '监控节点<br />机器IP：'+pm+'<br />节点状态：'+dmtGetMachineReqStatus(p.data.xrk_req_status);
                        if(p.data.xrk_req_status == 0 || p.data.xrk_req_status == 2)
                            s += '<br />响应耗时：' + p.data.xrk_response_ms + 'ms';
                        s += '<br />路由跳数：' + p.data.xrk_jumps;
                        break;
                    default:
                        s += '未知类型<br />';
                        break;
                }
                return s;
            }
        },
		title: {
			x: 'center',
            top : '20',
			text:'云框架系统实时网络拓扑'
		},
		series: [
			{
				type:'tree',
                top: '65',
                left: '5%',
                bottom: '5%',
                right: '70',
                roam :true,
                backgroundColor: 'transparent',
                layout : 'orthogonal',
                //layout : 'radial',
                expandAndCollapse : true,
                symbol: 'image://'+g_siteInfo.doc_path+'images/clt_server.png',
                symbolSize: [25,25],
				data:[],
                orient: 'LR',
                label: { 
                    color:'#fff', 
                    position: 'top', verticalAlign: 'middle', align:'middle' 
                },
                leaves: { 
                    label: { 
                        position: 'top', verticalAlign: 'middle', align:'middle' 
                    }
                },
                expandAndCollapse: true,
                initialTreeDepth : -1,
                animation : true,
                animationDuration: 0
			}
		]
	};

	var o_chart = echarts.init(document.getElementById(ct_id), g_dmtChartStyle);
    dmtSetTreeChartData(attr_info);
    op.series[0].data[0] = attr_info;
	o_chart.setOption(op);
    return o_chart;
}

function dmtShowGraphChart(attr_info, ct_id, o_chart)
{
	var op = {
        backgroundColor: 'transparent',
		title: {
			x: 'center',
			top:25,
            textStyle:{
                fontSize: 14,
            },
			show:false,
			text:'云框架系统实时网络拓扑图'
		},
		legend : [{
			type:'scroll',
			orient: 'horizontal',
			left:'left',
			top:'40',
			show:true
		}],
        animation: true,
		series: [
			{
				type:'graph',
                layout : 'circular',
                backgroundColor: 'transparent',
                edgeLabel: { fontSize: 20},
                symbolSize: 30,
				data:[],
                links:[],
                label: { show: true},
                roam: true,
                circular : {
                    rotateLabel : false 
                },
                force : {
                    initLayout:'circular',
                    edgeLength : 200,
                    repulsion: 100,
                    layoutAnimation:true,
                    draggable:false,
                    focusNodeAdjacency:true,
                    symbolRotate:70
                }
			}
		]
	};

    if(typeof o_chart != "undefined" && o_chart != null) 
	    o_chart.dispose();
	o_chart = echarts.init(document.getElementById(ct_id), g_dmtChartStyle);
    op.series[0].data = dmtGetTopData(attr_info.data, $('#'+ct_id).width(), $('#'+ct_id).height());
    op.series[0].links = attr_info.links;
	o_chart.setOption(op);
}

function dmtShowAttrInfo(attr_list, attr_val_list, ct_div, showtype)
{
    var op_series = {
		name:'',
        symbol : 'emptyCircle',
        showSymbol : false,
		cursor:'pointer',
		smooth: true,
		smoothMonotone:'x',
		type:'line',
        itemStyle : {  
            normal : {  
            },
            lineStyle:{
                normal:{
                    opacity:1
               }
            }
        },
        sampling : 'average',
        areaStyle: null,
        stack: null,
		data:[]
    };


	var op = {
        backgroundColor: 'transparent',
		legend: {
			show:false,
			bottom:10
		},
		title:{
			text:'',
			x:'left',
			subtext:'',
			top:5,
            subtextStyle: { 
                fontSize: 12,
                color:'red'
            },
            textStyle:{
                fontSize: 14
            },
			show:true
		},
		useUTC:true,
		toolbox: {
			show:true,
			orient: 'horizontal',
			top: 'top',
            zlevel :999,
			right:5,
			feature: {
                myPluginShowAttrMach: {
                    show : false,
                    title : '查看该监控点在各机器的上报',
					icon:'image://'+g_siteInfo.doc_path+'images/application_view_tile.png',
                    onclick : function() {}
                },
				myUnMaskWarningData:{
					show:true,
					title:'取消屏蔽提示',
					icon:'image://'+g_siteInfo.doc_path+'images/phone_sound.png',
					onclick:function(){}
				},
				myMaskWarningData:{
					show:true,
					title:'屏蔽提示',
					icon:'image://'+g_siteInfo.doc_path+'images/phone_delete.png',
					onclick:function(){}
				},
				mySetWarningData:{
					show:true,
					title:'设置提示',
					icon:'image://'+g_siteInfo.doc_path+'images/phone_edit.png',
					onclick:function(){}
				},
				myShowSingleData:{
					show:true,
					title:'单独显示图表数据',
					icon:'image://'+g_siteInfo.doc_path+'images/arrow_out_longer.png',
					onclick:function(){}
				},
				myShowDetailData:{
					show:true,
					title:'图表概要信息',
					icon:'image://'+g_siteInfo.doc_path+'images/database_table.png',
					onclick:function(){}
				},
				restore: { 
					show:true,
					title: '还原图表显示'
				},
				dataZoom:{
					show:true,
					xAxisIndex:0,
					yAxisIndex:false
				},
				saveAsImage: { show: false}
			}
		},
	    tooltip: {
            formatter: null,
			trigger: 'axis'
		},
        color: g_dmtChartColor,
		grid: {
            backgroundColor: 'transparent',
			tooltip: {
				show:true,
				trigger:'axis'
			},
            show:false,
			top:50,
            left:60,
            right:30,
			bottom:50
		},
	    xAxis: {
			show:true,
			type: 'time',
            splitLine: {
                show: false
            },
			axisPointer: {
			    label: {
			        formatter: function (params) {
			            if(attr_val_list.type != 1)
			                return dmtGetDateStr(params.value);
			            return  dmtGetDateStr(params.value, true);
			        }
			    }
			}
	    },
		yAxis: {
            splitLine: {
                show: false
            },
			splitArea: {
				show: false 
			},
			axisLabel:{
				formatter: null
			},
			show:true,
			type: 'value'
		},
		tooltip: {
			trigger: 'none',
			axisPointer: {
				type: 'cross',
				snap: true
			}
		},
		dataZoom: [
			{
				type:'inside',
                start: 0,
                end : 100,
	 			xAxisIndex: 0
		  	}
		],
		series: []
	};

	var attr_vals = attr_val_list.list;
	if(typeof attr_vals == "undefined")
		return;

	var jumpid;
	if(showtype == 'view')
		jumpid = attr_val_list.view_id;
	else  if(showtype == 'machine')
		jumpid = attr_val_list.machine_id;
    else if(showtype == 'pluginview')
		jumpid = attr_val_list.plugin_id;

	var dateStart = attr_val_list.date_time_start_utc*1000 - new Date().getTimezoneOffset()*60*1000;
	var count_day = 1;
	if(typeof attr_val_list.date_time_monday != 'undefined')
		count_day = 7;

	if(attr_val_list.type == 1)
	{
		op.legend.show = true;
		op.grid.bottom = 80;
	}

	var bHasData = true;
	for(var i=0; i < attr_vals.length; i++)
	{
		var attr_show_id = "attr_" + attr_vals[i].id + "_" + jumpid + showtype;
		if($('#'+attr_show_id).length > 0)
		{
			dmtJsBugReport('dmt.comm.js', 'dmtShowAttrInfo', 'bug:'+attr_show_id);
			$('#'+attr_show_id).html("");
		}

		var attr_show_container_str = '<div style="width:' + g_dmtChartWidth + 'px; ';
		attr_show_container_str += ' height:' + g_dmtChartHeight + 'px; ';
        attr_show_container_str += ' float:left; margin:' + g_dmtChartMargin + 'px;" ';
		attr_show_container_str += ' type="' + showtype + '" ';
		attr_show_container_str += ' id="' + attr_show_id+ '" ';
		attr_show_container_str += ' class="chart-DES-PWE" ';
		attr_show_container_str += ' type_id="' + jumpid + '" >';
		attr_show_container_str += '</div>';

		$(ct_div).append( $(attr_show_container_str) );

		var attr_info = attr_list.list[i];
		if(attr_info.id != attr_vals[i].id)
			attr_info = dmtGetAttrInfo(attr_list, attr_vals[i].id);

		if(typeof(g_all_charts[attr_show_id]) != "undefined") 
			g_all_charts[attr_show_id].dispose();
        var time_info = dmtGetXAxisTimeInfo(dateStart, count_day, attr_info, attr_vals[i].max_x);

		// 字符串型监控点
		if(attr_info.data_type == STR_REPORT_D_IP) {
			dmtSetStrAttrInfoChartIpGeo(attr_show_id, attr_info, attr_vals[i], attr_val_list, showtype);
			continue;
		}
		else if(attr_info.data_type == STR_REPORT_D) {
			dmtSetStrAttrInfoChart(attr_show_id, attr_info, attr_vals[i], attr_val_list, showtype);
			continue;
		}

        op.series.push(dmtCloneObj(op_series));
		if(typeof attr_vals[i].value_list_str != 'undefined' && attr_vals[i].max > 0) {
            if(attr_val_list.type == 1)
			    op.series[0].data = dmtGetYAxisData(time_info, attr_vals[i].value_list_str, true, attr_vals[i].max_x);
            else
			    op.series[0].data = dmtGetYAxisData(time_info, attr_vals[i].value_list_str);
			bHasData = true;

            if(attr_val_list.is_today && attr_vals[i].max_x > 0) {
                op.dataZoom[0].start = dmtGetStartXAxis(op.series[0].data, 30);
                if(op.dataZoom[0].start < 0)
                    op.dataZoom[0].start = 0;
                op.series[0].showSymbol = true;
            }
            else
                op.dataZoom[0].start = 0;
		}
		else  {
			op.series[0].data = [];
			bHasData = false;
		}

		if(attr_val_list.type == 1)
		{
            op.series.push(dmtCloneObj(op_series));
            op.series.push(dmtCloneObj(op_series));
			op.series[0].name = '今日 [' + attr_val_list.date_time_cur + ']';
			if(attr_vals[i].value_list_yst_str != "0")
				op.series[1].data = dmtGetYAxisData(time_info, attr_vals[i].value_list_yst_str, true, attr_vals[i].max_x);
			else
				op.series[1].data = [];
			op.series[1].name = '昨日 [' + attr_val_list.date_time_yst + ']';
			if(attr_vals[i].value_list_lwk_str != '0')
				op.series[2].data = dmtGetYAxisData(time_info, attr_vals[i].value_list_lwk_str, true, attr_vals[i].max_x);
			else
				op.series[2].data = [];
			op.series[2].name = '上周同日 [' + attr_val_list.date_time_wkd + ']';
			if(attr_vals[i].value_list_yst_str != "0" || attr_vals[i].value_list_lwk_str != '0')
				bHasData = true;
		}

		// 5 -- 历史积累监控点类型
		if(attr_info.data_type == SUM_REPORT_TOTAL)
			op.yAxis.min = attr_vals[i].min;
		else
			op.yAxis.min = 0;

		attr_val_list.show_single_type = attr_val_list.type;
		op.title.text = attr_info.name + '【' + attr_info.id + "】";

        // 图表概要信息存入 warnInfo 中
		var warnInfo = new Object();
		dmtSetViewChartInfo(showtype, attr_val_list, attr_vals[i], attr_info, warnInfo);
		dmtSetCustToolBox(op, showtype, attr_val_list, attr_vals[i], attr_info, warnInfo);

		if(!bHasData) {
            op.title.textStyle.fontSize = 16;
            op.title.subtextStyle.fontSize = 14;
            if(attr_vals[i].cur > 0) {
                var subtitle = "\n\n 当前时间：" + attr_val_list.date_time + ', ';
                subtitle += " 当前统计周期已上报：" + dmtShowChangeValue(attr_vals[i].cur) + ', ';
                subtitle += " 统计周期：" + dmtShowHumanStaticTime(attr_info.static_time) + "\n\n";
                op.title.subtext = subtitle;
            }
            else
                op.title.subtext = '\n\n暂无数据上报';
			op.title.top = '40%';
			op.title.left = 'center';
			op.xAxis.show = false;
			op.yAxis.show = false;
			op.toolbox.feature.dataZoom.show = false;
			op.toolbox.feature.myShowSingleData.show  = false;
		}
		else {
            op.title.textStyle.fontSize = 14;
            op.title.subtextStyle.fontSize = 12;
            op.title.subtext = '';
			op.title.top = 5;
            op.title.left = 10;
			op.xAxis.show = true;
			op.yAxis.show = true;
			op.toolbox.feature.dataZoom.show = true;
            if(showtype != 'pluginview')
			    op.toolbox.feature.myShowSingleData.show  = true;
            else
			    op.toolbox.feature.myShowSingleData.show  = false;
		}

		g_all_charts[attr_show_id] = echarts.init(document.getElementById(attr_show_id), g_dmtChartStyle);
        dmtSetChartOptionByAttrInfo(attr_info, op, {showtype:showtype, day_type:attr_val_list.type});
		g_all_charts[attr_show_id].setOption(op);

		// echarts toolbox onclick 函数一旦设置后不能改变，所有将提示相关参数保存到数组对象中
		g_all_charts[attr_show_id].warnInfo = warnInfo;
		g_all_charts[attr_show_id].setwidth = g_dmtChartWidth;
	}
}

function dmtChartContainerEmpty(ctdiv)
{
    var nhtml = '<font color="red" style="font-size:16px;font-weight:bold;">无监控点或监控点暂无数据上报</font>';
    $(ctdiv).css('padding-top', '20px').html(nhtml);
}

function dmtGetSyssetTypeHtml(shtmlid)
{
	var html = '<label>服务器类型：</label>';
	html += '<select name="';
	html += shtmlid + '" id="';
	html += shtmlid + '">" ';
	html += '<option value="0">请选择</option>';
	html += '<option value="1">日志服务器</option>';
	html += '<option value="2">监控点服务器</option>';
	html += '<option value="3">mysql 监控点服务器</option>';
	html += '<option value="4">中心服务器</option>';
	html += '<option value="11">web 服务器</option>';
	html += '</select>';
	g_html_systype = html;
	return g_html_systype;
}

function dmtGetServerTypeName(type_id)
{
	switch(type_id)
	{
		case 1:
			return "日志服务器";
		case 2:
			return "监控点服务器";
		case 3:
			return "mysql 监控点服务器";
		case 4:
			return "中心服务器";
		case 11:
			return "web 服务器";
		default:
			return "未知类型";
	}
}


function dmtTriggerLeftMenu()
{
	if(DWZ.ui.sbar == true)
	{
		$("#sidebar .toggleCollapse div").trigger("click");
		return;
	}
	
	if(DWZ.ui.sbar == false)
	{
		$("#sidebar_s .toggleCollapse div").trigger("click");
		return;
	}
}

function dmtLv2CheckCodeDlgClose()
{
	if(typeof(g_dclc_TimerId) != "undefined" && g_dclc_TimerId != null)
	{
		clearTimeout(g_dclc_TimerId);
		g_dclc_TimerId = null;
	}
	return true;
}

function dmtPopDaemonTipMsg()
{
	if(typeof($.pdialog._current) != "undefined" && $.pdialog._current != null)
	    $.pdialog.closeCurrent();
	var op = { mask:true, maxable:false, height:280, width:410, resizable:false, drawable:true };
	$.pdialog.openLocal('dct_dlg_show_daemon_tip_msg', 'dct_dlg_show_daemon_tip_msg', '演示版操作提示', op);
}

// ajax 返回
// 该函数返回 true 则后续逻辑不处理, 返回 false 继续处理
function dmtFirstDealAjaxResponse(result)
{
	if(typeof(result) == "undefined")
	{
		return false;
	}

	if(result == null || result == 'null' || typeof(result.ec) == "undefined")
		return false;

	if(result.ec == 666) {
		dmtPopDaemonTipMsg();
		return true;
	}

	if(result.ec == 111) {
		navTab.closeCurrentTab(); 
		if(typeof($.pdialog._current) != "undefined" && $.pdialog._current != null)
			$.pdialog.closeCurrent();
		// 登录过期重新登录
		location = result.redirect_url;
		return true;
	}

	if(result.ec == 300  || (result.ec >= 1 && result.ec < 200)) {
		var msg = '服务器返回错误，错误码：';
		msg += result.ec;
		if(typeof(result.msg) != "undefined")
		{
			msg += '，错误消息：';
			msg += result.msg;
		}
		else if(typeof(result.msgid) != "undefined")
		{
			msg += '，错误消息：';
			msg += DWZ.msg(json.msgid);
		}
		alertMsg.warn(msg);
		return true;
	}

	return false;
}

function dmtGetHumanReadFixed2ByKB(value)
{
    return dmtGetHumanReadFixed2(value*1024);
}

function dmtGetHumanReadFixed2(value)
{
    if(value >= 1073741824)
    {
        var f = value/1073741824;
        return f.toFixed(2) + 'G';
    }
    if(value >= 1048576)
    {
        var f = value/1048576;
        return f.toFixed(2) + 'M';
    }
    if(value >= 1024)
    {
        var f = value/1024;
        return f.toFixed(2) + 'K';
    }
    return value;
}

function dmtGetHumanReadDigitByKB(val)
{
	var sp = '';
	if(val >= 1048576){
		var f = val / 1048576;
		sp = Math.round(parseFloat(f)*100)/100 + ' GB';
	}
	else if(val >= 1024) {
		var f = val / 1024;
		sp = Math.round(parseFloat(f)*100)/100 + ' MB';
	}
	else if(typeof val != 'undefined')
		sp = val + ' KB';
	return sp;
}

function dmt_duc_dlg_modify_name_close()
{
	if(typeof g_dci_TimerId != 'undefined' && g_dci_TimerId != null)
	{
		clearTimeout(g_dci_TimerId);
		g_dci_TimerId = null;
	}
	return true;
}

function dmtGetFastWebIdx()
{
	if(typeof r_siteInfo == 'undefined')
		return 0;

	r_siteInfo.web_fast_ms = 24*60*60*1000;
	r_siteInfo.web_slow_ms = 0;

	var ips = r_siteInfo.web_list;
	var fastIdx = ips.length;
	for(var i=0; i < ips.length; i++)
	{
		if(typeof ips[i].end == 'undefined')
			continue;

		var s = ips[i].end.getTime() - ips[i].start.getTime();
		if(s > r_siteInfo.web_slow_ms)
			r_siteInfo.web_slow_ms = s;
		if(s < r_siteInfo.web_fast_ms)
		{
			r_siteInfo.web_fast_ms = s;
			fastIdx = i;
		}
	}
	return fastIdx;
}

function dmtTestWebSvr()
{
	if(typeof r_siteInfo == 'undefined')
		return 0;

	for(var i=0; i < r_siteInfo.web_list.length; i++)
	{
		r_siteInfo.web_list[i].start = new Date();
		var requrl = 'http://' + r_siteInfo.web_list[i].ip + '/cgi-bin/slog_flogin?action=check_web_speed';
		requrl += '&check_idx=' + i;
		$.ajax({
			url:requrl,
			cache:false,
			success:function(result){
				if(result.check_idx >= 0 && result.check_idx < r_siteInfo.web_list.length)
					r_siteInfo.web_list[ result.check_idx ].end = new Date();
			},
			dataType:'json'
		});
	}
}

function dmtGetBytesLength(str)
{
	var total = 0;
	for(var i=0, len = str.length; i < len; i++)
	{
	    charCode = str.charCodeAt(i);
	    if(charCode <= 0x007f) {
	        total += 1;
	    }else if(charCode <= 0x07ff){
	        total += 2;
	    }else if(charCode <= 0xffff){
	        total += 3;
	    }else{
	        total += 4;
	    }
	}
	return total;
}

// ysy add - 2019-01-31
function redrawChartsOnSwitchTab()
{
	var nid = navTab.getCurrentTabId();
	if(nid != null) {
		var type_id;
		var type;
		var isLeftShow;
		if(nid.match('showmachine_')) {
			type = 'machine';
			type_id = nid.split('_')[1];
			if($('#dsmLeftMenu_'+type_id).css('display') == 'none')
				isLeftShow = false;
			else
				isLeftShow = true;
		}
		else if(nid.match('showplugin_')) {
			type = 'plugin';
			type_id = nid.split('_')[1];
			isLeftShow = false;
		}
		else if(nid.match('showview_')) {
			type = 'view';
			type_id = nid.split('_')[1];
			if($('#dsLeftMenu_'+type_id).css('display') == 'none')
				isLeftShow = false;
			else
				isLeftShow = true;
		}
		else if(nid.match('pluginview_')) {
			type = 'pluginview';
			type_id = nid.split('_')[1];
			if($('#dsPluginLeftMenu_'+type_id).css('display') == 'none')
				isLeftShow = false;
			else
				isLeftShow = true;
		}
		else if(nid.match('pluginattr_')) {
			type = 'pluginattr';
			type_id = nid.split('_')[1];
			if($('#ditsPluginLeftMenu_'+type_id).css('display') == 'none')
				isLeftShow = false;
			else
				isLeftShow = true;
		}
		else  {
			return;
		}
		dmtSetRedrawChartsInfo(type_id, type, isLeftShow);
		dmtRedrawCharts();
	}
}

// ysy add - 2019-02-08 --- 主页图表大小自适应
function redrawChartsMain()
{	
	// 当前tab 是主页
	if($('#navMenuLast').hasClass('selected')) {
 	   	dcMainPageSetChartSize();
 	   	dcMainPageRedrawChart();
	}
}

// ysy add - 2019-01-30 --- 单机视图图表大小自适应
function redrawCharts()
{
	var nid = navTab.getCurrentTabId();
	if(nid != null) {
		if(nid.match('showmachine_') || nid.match('showview_') || nid.match('showplugin_')
            || nid.match('pluginview_') || nid.match('pluginattr_')) 
        {
			dmtRedrawCharts();
		}
		if(nid.match("dmt_plugin_")) {
		    dmtSetPluginMarginInfo(nid);
		}
	}
	else {
		// 主页
		dcOnMainPage('main');
	}
}

function dmtGetAttrTypeInfoById(attr_type_id)
{
    if(g_attr_type_list == null)
        return null;

    var attr_type = dmtGetTypeInfo(g_attr_type_list.user_self_cust, attr_type_id);
    if(attr_type != 'null')
        return attr_type;

    attr_type = dmtGetTypeInfo(g_attr_type_list.global_plug, attr_type_id);
    if(attr_type != 'null')
        return attr_type;
    return null;
}

function dmtGetAttrTypeNameById(attr_type_id)
{
    if(g_attr_type_list == null)
        return '未加载类型信息';
    var t = dmtGetAttrTypeInfoById(attr_type_id);
    if(t != null)
        return t.name;
    return '监控点类型查找失败';
}

function dmtAddNoDataTextForView(view_attr_ct, ct_div)
{
	var sel = '#' + view_attr_ct + ' a[ctid]';
    var hasChart = false;
	$(sel).each(function() {
		var ctsel = '#' + $(this).attr('ctid');
		if($(ctsel).length <= 0) {
    		var showno = $(this).html();
            $(this).html("<font class='chart_has_no_data_rep' color='blue'>&nbsp;[无上报]&nbsp;</font>" + showno);
		}
        else
            hasChart = true;
	});
    if(typeof ct_div != 'undefined' && !hasChart)
        dmtChartContainerEmpty('#'+ct_div);
    else {
        $('#' + view_attr_ct + ' .chart_has_no_data_rep').each(function() {
            $(this).parent().parent().hide();
        });
    }
}

var dmtAddEvent=(function(){
    var _eventCompat=function(event){
        var type=event.type;
        if(type=='DOMMouseScroll'||type=='mousewheel'){
            event.delta=(event.wheelDelta)?event.wheelDelta/120:-(event.detail||0)/3;
        }
        if(event.srcElement&&!event.target){
            event.target=event.srcElement;    
        }
        if(!event.preventDefault&&event.returnValue!==undefined){
            event.preventDefault=function(){
                event.returnValue=false;
            };
        }
        return event;
    };
  
    if(window.addEventListener){
        return function(el, type, fn, capture){
            if(type==="mousewheel"&&document.mozHidden!==undefined){
                type="DOMMouseScroll";
            }
            el.addEventListener(type, function(event){
                fn.call(this, _eventCompat(event)); }, capture||false);
        }
    } 
    else if(window.attachEvent){
        return function(el, type, fn, capture){
            el.attachEvent("on" + type, function(event){
                event = event || window.event;
                fn.call(el, _eventCompat(event));    
            });
        }
    }
    return function(){};    
})(); 

function dmtSortTable(selector, compFunc) 
{
    var mySelector = 'th.xrk_sortable';
    var myCompFunc = function($td1, $td2, isAsc) {
        if(typeof($td1.attr('src_val')) != 'undefined' && typeof($td2.attr('src_val')) != 'undefined') {
            var v1=Number($td1.attr('src_val'));
            var v2=Number($td2.attr('src_val'));
            return isAsc ? v1 > v2 : v1 < v2;
        }
        var v1 = $.trim($td1.text()).replace(/,|\s+|%/g, '');
        var v2 = $.trim($td2.text()).replace(/,|\s+|%/g, '');
        var pattern = /^\d+(\.\d*)?$/;
        if (pattern.test(v1) && pattern.test(v2)) {
            v1 = parseFloat(v1);
            v2 = parseFloat(v2);
        }
        return isAsc ? v1 > v2 : v1 < v2;
    };

    var doSort = function($tbody, index, compFunc, isAsc)
    {
        var $trList = $tbody.find("tr");
        var curSortIdx = $tbody.attr('sort_idx');
        var len = $trList.length;

        if(typeof curSortIdx == 'undefined')  {
            for(var i=0; i<len; i++) {
                var $td = $trList.eq(i).find("td").eq(index);
                var txt = $td.text();
                $td.html('<span class="xrk_text_bl">' + txt + '</span>');
            }
            $tbody.attr('sort_idx', index);
        }
        else if(curSortIdx != index) {
            for(var i=0; i<len; i++) {
                var $tdold = $trList.eq(i).find("td").eq(curSortIdx);
                var txtold = $tdold.text();
                $tdold.html(txtold);

                var $td = $trList.eq(i).find("td").eq(index);
                var txt = $td.text();
                $td.html('<span class="xrk_text_bl">' + txt + '</span>');
            }
            $tbody.attr('sort_idx', index);
        }

        for(var i=0; i<len; i++) {
            var $td = $trList.eq(i).find("td").eq(index);
            var txt = $td.text();
            $td.html('<span class="xrk_text_bl">' + txt + '</span>');
        }

        for(var i=0; i<len-1; i++) {
            for(var j=0; j<len-i-1; j++) {
                var $td1 = $trList.eq(j).find("td").eq(index);
                var $td2 = $trList.eq(j+1).find("td").eq(index);
                if (compFunc($td1, $td2, isAsc)) {
                    var t = $trList.eq(j+1);
                    $trList.eq(j).insertAfter(t);
                    $trList = $tbody.find("tr");
                }
            }
        }
    }

    var init = function(pselector) {
        if(typeof pselector == 'string')
            var $th = $(pselector);
        else
            var $th = pselector;
        var $ptable = $th.closest("table");
        $th.click(function(){
                var index = $(this).index();
                var isAsc = $(this).hasClass('asc');
                doSort($ptable.find("tbody"), index, compFunc, isAsc);
                $(this).toggleClass('asc').toggleClass('desc');
            });
        $th.attr('title', '点击排序');
    };

    selector = selector || mySelector;
    compFunc = compFunc || myCompFunc;
    init(selector);
}

