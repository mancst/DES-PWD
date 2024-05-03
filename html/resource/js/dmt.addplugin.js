// in dmt_dlg_machine_install_plugin.html use
var g_addplugin_timerid = 0;

// in dmt_dlg_machine_opr_plugin.html use
var g_dmop_timerid = 0;

// 引用 dmt_dp_add_plugin.html 中的 g_ddap_machlist
function dmapGetMachineInfo(mach_id)
{
    if(typeof g_ddap_machlist == 'undefined')
        return ' error, not find machine list ! ';
    for(var i=0; i < g_ddap_machlist.count; i++) {
        if(g_ddap_machlist.list[i].id == mach_id) {
            return '机器：' + g_ddap_machlist.list[i].name + ' (IP:' + g_ddap_machlist.list[i].ip + ')';
        }
    }
    return '机器ID:' + mach_id + '(未找到相关信息)';
}

function dmapGetOprText(st)
{
    var txt = '未知状态';
    switch(st) {
        case 1:
            txt = '错误：机器未安装该插件';
            break;
        case 2:
            txt = '最近5分钟内有未完成的操作, 请稍后再试';
            break;
        case 3:
            txt = '操作中，请稍等 ...';
            break;
        case 4:
            txt = '服务器发生错误，请稍后再试';
            break;
        case 5:
            txt = '未找到操作任务';
            break;
        case 6:
            txt = '操作超时(30s 没有完成)';
            break;
        case 7:
            txt = 'agent 未启动，任务无法处理';
            break;
        case 8:
            txt = 'agent 在机器上未找到该插件';
            break;
        case 12:
            txt = '已发布任务';
            break;
        case 13:
            txt = '任务已下发到机器';
            break;
        case 14:
            txt = '操作失败，任务未完成';
            break;
        case 15:
            txt = '操作成功，已完成';
            break;
        case 16:
            txt = 'agent 响应超时';
            break;
    }
    return txt;
}

function dmapAddPlugin_reload()
{
    navTab.reload();
    if(g_addplugin_timerid != 0) {
        clearInterval(g_addplugin_timerid);
        g_addplugin_timerid = 0;
    }

    if(g_dmop_timerid != 0) {
        clearInterval(g_dmop_timerid);
        g_dmop_timerid = 0;
    }
    return true;
}

function ddapMachOprPlugin(cgip, machs, plugid, optype)
{
	var url = cgip + "mt_slog?action=ddap_multi_opr_plugin&machines=" + machs;
    url += "&plugin=" + plugid;
    url += "&opr_type=" + optype;
	var op = {"mask":true,"maxable":false,"height":460,"width":600, "close":dmapAddPlugin_reload}; 
    var tl = '';
    switch(optype) {
        case 1:
            op.width = 700;
            op.height = 520;
            tl = '批量修改插件配置';
            break;
        case 2:
            tl = '批量移除插件';
            break;
        case 3:
            tl = '批量启用插件';
            break;
        case 4:
            tl = '批量禁用插件';
            break;
    }
	$.pdialog.open(url, "dlg_ddap_mach_opr_plugin", tl, op); 
}


function dmapMachOprPlugin(cgip, mach, plugs, optype)
{
	var url = cgip + "mt_slog?action=dmap_multi_opr_plugin&machine=" + mach;
    url += "&plugins=" + plugs;
    url += "&opr_type=" + optype;
	var op = {"mask":true,"maxable":false,"height":460,"width":600, "close":dmapAddPlugin_reload}; 
    var tl = '';
    switch(optype) {
        case 2:
            tl = '批量移除插件';
            break;
        case 3:
            tl = '批量启用插件';
            break;
        case 4:
            tl = '批量禁用插件';
            break;
    }
	$.pdialog.open(url, "dlg_dmap_mach_opr_plugin", tl, op); 
}

function dmapModMachPlugin(cgip, machine_id, plugid)
{
	var url = cgip + "mt_slog?action=ddap_multi_opr_plugin&machines=" + machine_id;
    url += "&plugin=" + plugid;
    url += "&opr_type=1";
	var op = {"mask":true,"maxable":false,"height":520,"width":700, "close":dmapAddPlugin_reload};
	$.pdialog.open(url, "dlg_ddap_mach_opr_plugin", '修改插件配置', op); 
}

function dmapAddPlugin(cgip, machid, plugid, inner)
{
	var url = cgip + "mt_slog?action=ddap_install_plugin&mach=" + machid;
    url += "&plugin=" + plugid;
	url += "&self_domain=" + window.document.domain;
    url += "&is_inner_mode=" + inner;
	var op = {"mask":true,"maxable":false,"height":480,"width":600, "close":dmapAddPlugin_reload}; 
    if(inner)
    	$.pdialog.open(url, "dlg_ddap_install_plugin", "内网模式一键部署插件进度", op); 
    else 
    	$.pdialog.open(url, "dlg_ddap_install_plugin", "外网模式一键部署插件进度", op); 
}

function dmtAddPluginStatusShow(step, rid, wid)
{
    switch(step) {
        case 1:
            $('#'+rid).append('<li>已发布任务</li>');
            break;
        case 2:
            $('#'+rid).append('<li>已分派任务</li>');
            break;
        case 3:
            $('#'+rid).append('<li>接入服务已接收任务</li>');
            break;
        case 4:
            $('#'+rid).append('<li>接入服务已下发任务到机器</li>');
            break;
        case 5:
            $('#'+rid).append('<li>agent 已向云服务查询到安装包下载地址</li>');
            break;
        case 6:
            $('#'+rid).append('<li>agent 已下载插件部署包</li>');
            break;
        case 7:
            $('#'+rid).append('<li>agent 已部署插件并启动</li>');
            break;
        case 8:
            $('#'+rid).append('<li>接入服务已收到插件消息，插件部署成功</li>');
            break;
        case 9:
            $('#'+rid).append('<li>agent 已部署插件，部署完成</li>');
            break;

        case 15:
            $('#'+rid).append('<li>开始从云端获取下载地址</li>');
            break;
        case 16:
            $('#'+rid).append('<li>已获取下载地址</li>');
            break;
        case 17:
            $('#'+rid).append('<li>已下载插件部署包</li>');
            break;

        case 20:
            $('#'+wid).html('<font color="red">部署失败：agent 获取插件部署包地址失败, 请检查机器是否安装 wget 工具');
            break;
        case 21:
            $('#'+wid).html('<font color="red">部署失败：agent 获取插件部署包地址, 返回错误码');
            break;
        case 22:
            $('#'+wid).html('<font color="red">部署失败：agent 获取插件部署包地址, 返回内容解析错误');
            break;
        case 23:
            $('#'+wid).html('<font color="red">部署失败：agent 下载插件部署包失败');
            break;
        case 24:
            $('#'+wid).html('<font color="red">部署失败：agent 解压缩部署包失败, 请检查机器是否安装 tar 工具');
            break;
        case 25:
            $('#'+wid).html('<font color="red">部署失败：agent 创建部署目录失败, 请 agent 是否有目录创建权限');
            break;
        case 26:
            $('#'+wid).html('<font color="red">部署失败：agent 启动插件失败, 可能是插件与系统不兼容导致');
            break;
        case 27:
            $('#'+wid).html('<font color="red">部署失败：agent 下载开源版配置文件失败');
            break;
        case 28:
            $('#'+wid).html('<font color="red">部署失败：专有配置文件下载失败');
            break;
        case 29:
            $('#'+wid).html('<font color="red">部署失败：agent 响应超时');
            break;
        case 30:
            $('#'+wid).html('<font color="red">部署失败：解析下载文件错误');
            break;
        case 31:
            $('#'+wid).html('<font color="red">部署失败：生成插件部署包文件错误');
            break;
        case 32:
            $('#'+wid).html('<font color="red">部署失败：获取本地下载地址错误');
            break;
        case 33:
            $('#'+wid).html('<font color="red">部署失败：获取加密 key 错误');
            break;
 
        default:
            $('#'+wid).html('<font color="red">部署失败，进度错误：' + step);
            break;
    }
}

function dmtSetPluginShowMainTable(log_server_ip, log_server_port, plugin_id, show_main_tables, funcb)
{
	var cgi_path; 
	if(typeof g_siteInfo.cgi_path != 'undefined' && g_siteInfo.cgi_path != '')
		cgi_path = g_siteInfo.cgi_path;
	else
		cgi_path = '/cgi-bin/';
    var rurl = cgi_path + '/mt_slog?';
	var para = new Object();
    if(log_server_ip != 'null') {
	    rurl = 'http://'+log_server_ip + ':' + log_server_port + cgi_path + '/mt_slog?';
        para.ex_flogin_user = $.cookie("flogin_user");
        para.ex_flogin_md5 = $.cookie("flogin_md5");
        para.ex_flogin_type = $.cookie("flogin_type");
        para.ex_flogin_uid = $.cookie("flogin_uid");
        para.ex_flogin_index = $.cookie("flogin_index");
    }

    para.action = 'set_plugin_show_main_tables';
    para.plugin_id = plugin_id;
    para.tables = show_main_tables.join(' ');

    $.ajax({
        type: "post",
        url: rurl,
        data: para,
        success: function(js) {
            if(dmtFirstDealAjaxResponse(js))
                return;
            funcb(js);
        },
		timeout: function(){
            funcb(null);
        },
		error: function() {
            funcb(null);
		},  
        dataType: 'json', 
        global: false
    });
}

function dmtGetPluginTablesInfo(log_server_ip, log_server_port, plugin_id, funcb)
{
	var cgi_path; 
	if(typeof g_siteInfo.cgi_path != 'undefined' && g_siteInfo.cgi_path != '')
		cgi_path = g_siteInfo.cgi_path;
	else
		cgi_path = '/cgi-bin/';

	var para = new Object();
    var rurl = cgi_path + '/mt_slog?';
    if(log_server_ip != 'null') {
	    rurl = 'http://'+log_server_ip + ':' + log_server_port + cgi_path + '/mt_slog?';
        para.ex_flogin_user = $.cookie("flogin_user");
        para.ex_flogin_md5 = $.cookie("flogin_md5");
        para.ex_flogin_type = $.cookie("flogin_type");
        para.ex_flogin_uid = $.cookie("flogin_uid");
        para.ex_flogin_index = $.cookie("flogin_index");
    }
    para.action = 'get_plugin_tables_data';
    para.plugin_id = plugin_id;

    $.ajax({
        type: "post",
        url: rurl,
        data: para,
        success: function(js) {
            if(dmtFirstDealAjaxResponse(js))
                return;
            funcb(js);
        },
		timeout: function(){
            funcb(null);
        },
		error: function() {
            funcb(null);
		},  
        dataType: 'json', 
        global: false
    });
}

