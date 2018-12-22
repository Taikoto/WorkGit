#!/usr/bin/perl

# 打包 bin 为 zip 文件, 名称例如: j7296_20140507.zip
# 新建项目时，需要指定项目名称 $prj_name
# 传入参数为“0”时，不打包数据库
# PARRY 19:51 2014-05-07
# PARRY 13:50 2014-05-16 增加打包数据库文件 
# PARRY 16:23 2017-03-18 增加 MT6735 数据库目录，纠正搜索文件算法错误
# PARRY 17:36 2018/07/21 在被打包文件中找出最新的文件，并以此文件的修改时间为命名，避免重复打包
#                        缺少文件时, 不执行打包
# PARRY 17:22 2018/07/31 自动查找 Scatter 文件
# PARRY 09:20 2018/08/01 增加 7z 压缩，使打包文件最多再减少 111m
# PARRY 15:04 2018/12/06 增加参数，实现导出 OTA 文件到发布目录

package ParryMTKAndroidUtil;
require Exporter;
@ISA = qw(Exporter);

# @EXPORT 可被外部程序访问的函数, 其它未定义的都是私有, 不可被外部访问
@EXPORT = qw(package_bin);
# @EXPORT_OK = qw($myvar1 $myvar2);	 # 可被外部程序访问的变量

use Time::Piece;                     # strftime
use File::Path qw(mkpath rmtree);    # 创建、删除文件夹树
use File::Copy;                      # move
# use Data::Dumper;
use File::Spec::Functions;
use Cwd;
use utf8;
use ParryUtil;					# 提供实用函数, 如: 输出结果文件, 输出 Log
use ParryCopyFolderUtil;        # copyFile 复制文件

# ParryUtil::ClearScreen();       # 清屏 

################
my $g_util = ParryUtil->new();         	 # 提供实用函数

my $debug = 0;              # 调试模式
my $performanceChk  = 1;    # 性能检测
my $useBinFileLastModifyTime = 1;    # 构成压缩包文件名的时间字符串使用 Bin 最新的文件时间
my $FILE_LOG;
my $curdir = getcwd();      # 当前脚本目录
my @compress_method = ('linux-zip', '7zip-zip-x0', '7zip-zip-x1', '7zip-7z-x1', '7zip-7z-x5');
if ($debug)
{
    $FILE_LOG    = $g_util->open_text_file( '>', '~ParryMTKAndroidUtil.log');
    $g_util->set_log_handle($FILE_LOG) if ($debug);                        # 设置输出的 log 文件
}

sub package_bin
{
    my ($p_prj_name, $p_dir_bin, $p_dir_out, $p_build_mode, $p_include_database, $p_mtk_product, $p_include_ota, $p_method_id) = @_;
    if (!defined($p_method_id)) {
        $p_method_id = '';
    }

    mylogln("[package_bin] " 
        . "p_prj_name: " . $p_prj_name
        . ", p_dir_bin: " . $p_dir_bin
        . ", p_dir_out: " . $p_dir_out
        . ", p_build_mode: " . $p_build_mode
        . ", p_include_database: " . $p_include_database
        . ", p_mtk_product: " . $p_mtk_product
        . ", p_include_ota: " . $p_include_ota
        . ", p_method_id: " . $p_method_id ); 

    my $author = ParryUtil->getSysUserName();   # 作者名称
    my $l_error = 0;
    my @l_filelist = ();    # 将要被打包的文件
    my @l_not_exist = ();   # 缺少的文件

    my $temp_archive_fullpath = "";
    my $archive_name = "";
    my $retValue = "";

    # 数据库文件路径
    # 数据库目录及名称规则，不同平台，数据库文件所在目录可能不同，可自行添加
    my @l_database_dirs = (
        # 'mediatek/cgen',
        # "mediatek/config/out/$p_prj_name_mtk/modem",
        "$p_dir_bin/obj/ETC",                  # MT6735, MT6737, etc/mddb 的文件
        "$p_dir_bin/obj/CGEN",                  # MT6735, MT6737
    );

    my @l_database_file_regulation = (
        'APDB_',
        'BPLGUInfoCustomAppSrcP_',
    );

    # OTA 文件目录
    my @l_ota_dirs = (
        "$p_dir_bin",
        "$p_dir_bin/obj/PACKAGING/target_files_intermediates",
    );

    showMessageLn("==================================================");
    showMessageLn("打包 bin 文件");
    showMessageLn("==================================================");

    if (!defined($p_prj_name) or $p_prj_name eq '') {
        output_error("项目名称不能为空，无法打包! ");
        $l_error = 1;
    }

    if (!-d $p_dir_bin)
    {
        output_error("Bin 目录不存在，无法打包! $p_dir_bin");
        $l_error = 1;
    }
    
    if (!$l_error && (! -d $p_dir_out))
    {
        $l_error = !mkpath($p_dir_out);
        if ($l_error)
        {
            output_error("创建输出目录失败! $p_dir_out");                       # 创建目录失败
        }
    }
    
    my $fname_scatter = '';
    my $fname_scatter_fullpath = '';
    if (!$l_error) {
        $fname_scatter = find_scatter($p_dir_bin);
        if ($fname_scatter eq '') {
            output_error("Scatter 文件不存在，无法打包! $fname_scatter_fullpath");
            $l_error = 1;
        } else {
            $fname_scatter_fullpath = catfile( $p_dir_bin, $fname_scatter );
            if (!-f $fname_scatter_fullpath) {
                output_error("Scatter 文件不存在，无法打包! $fname_scatter_fullpath");
                $l_error = 1;
            }
        }
    }

    # build.prop 文件的位置
    my $build_prop_file_path = catfile($p_dir_bin, 'system'); 
    $build_prop_file_path = catfile($build_prop_file_path, 'build.prop');
    showMessageLn('build.prop 文件: ' . $build_prop_file_path . (-f $build_prop_file_path ? '' : ' (不存在)'));

    if (!$l_error){
        mylogln("当前文件夹: $curdir");
        mylogln("Bin 文件夹: $p_dir_bin");
        mylogln("Scatter 文件: $fname_scatter_fullpath"); 
        showMessageLn("Scatter 文件: $fname_scatter_fullpath"); 
    
        # Scatter
        mylogln("\n添加 Scatter 文件..."); 
        push(@l_filelist, $fname_scatter_fullpath);
        
        # Bin
        mylogln("\n添加 Bin 文件..."); 
        my @l_filelist_bin = parsing_scatter($fname_scatter_fullpath);
        foreach my $fname_temp (@l_filelist_bin) {
            $fname_temp = catfile( $p_dir_bin, $fname_temp );
            if (-f $fname_temp) {
				push(@l_filelist, $fname_temp);
                mylogln("$fname_temp"); 
            } else {
               push(@l_not_exist,$fname_temp);
            }
        }

        # 数据库文件
        if ($p_include_database) {
            mylogln("\n添加数据库文件..."); 
            my @database_files;
            foreach my $dir (@l_database_dirs){
                push (@database_files, add_db($dir, @l_database_file_regulation));
            }
            
            # 复制数据库文件到 bin 目录
            foreach my $l_from_file (@database_files){
                my $fname_temp = just_fname_stem($l_from_file);
                my $l_to_file = catfile( $p_dir_bin, $fname_temp );
                copyFile($l_from_file, $l_to_file);
            }
            
            push(@l_filelist, @database_files);
        }

        # 列出缺少的文件
        if (@l_not_exist > 0)
        {
            $l_error = 1;
            showMessageLn("*** 缺少以下文件, 无法打包:"); 
            foreach my $fname_temp (@l_not_exist)
            {
                showMessageLn("$fname_temp"); 
            }
        } else {
            # 生成压缩文件名称
            my $c_date_time = getTimeStrForArchiveName();       # 以当前时间
            if ($useBinFileLastModifyTime) {
                # 对文件按修改时间排序, 最新的文件排在最前, 找出最新的时间
                my @sortfiles = sort{
                    @sta1=stat($a);
                    @sta2=stat($b);
                    $sta2[9]<=>$sta1[9];    # 降序
                    # $sta1[9]<=>$sta2[9];    # 升序
                } @l_filelist;

                if ($debug){
                    mylogln('按文件修改时间排序:'); 
                    foreach my $file (@sortfiles){
                        $date = getTimeStr((stat $file)[9]);
                        mylogln($date . ' ' . $file); 
                    }
                }

                my $mtime = (stat($sortfiles[0]))[9];   # 最新的文件排在最前
                $c_date_time = getTimeStrForArchiveName($mtime);  # 以被打包文件中最新的修改时间
            }

            # 默认压缩方法
            my $method_id = $p_method_id;
            if (!$method_id) {
                # $method_id = 'linux-zip';     # 487m 24 秒 （兼容性好）
                # $method_id = '7zip-zip-x0';   # 896m 4 秒  （只存储，不压缩，兼容性好）
                # $method_id = '7zip-zip-x1';   # 471m 28 秒 （兼容性好）
                # $method_id = '7zip-7z-x1';    # 421m 7 秒  （快速）
                $method_id = '7zip-7z-x5';    # 376m 31 秒   （最小，推荐）
            }

            if (!check_compress_method($method_id)){
                output_error('当前压缩方法：' . $method_id . ', 压缩方法必须为: ' . join(", ", @compress_method)) or die;
            }

            # 读取 bin 版本号
            my $bin_version = '';
            if (-f $build_prop_file_path) {
                my $FILE_BUILD_PROP = $g_util->open_text_file( '<', $build_prop_file_path );
                my @build_prop = <$FILE_BUILD_PROP>;
                close($FILE_BUILD_PROP);
                
                foreach my $line (@build_prop) {
                    # ro.build.display.id=V002
                    mylog($line);
                    if ($line =~ /^\s*ro\.build\.display\.id=\s*(.*)\s*/i) {
                        $bin_version = $1;
                        last;
                    }
                }
            }

            my $archive_base_name =  sprintf("%s%s_%s_%s_%s", $p_prj_name, '_' . $bin_version, $p_build_mode, $author, $c_date_time);
            $archive_name = build_archive_name($method_id, $archive_base_name);
            my $archive_fullpath = catfile($p_dir_out, $archive_name);
            
            # 执行压缩
            showMessageLn("压缩包: $archive_fullpath"); 
            if (!-f $archive_fullpath){
                showMessageLn("正在压缩..."); 

                # 移除目录，只留下文件名，因为 7z 会把路径加入到压缩包中
                my @file_list_for_compress = ();
                foreach my $file (@l_filelist)
                {
                    my $fname_temp = just_fname_stem($file);
                    mylogln($fname_temp);
                    push(@file_list_for_compress, $fname_temp);
                }

                # 压缩文件
                $l_error = !compress($method_id, $archive_name, $p_dir_bin, @file_list_for_compress);
                $temp_archive_fullpath = catfile($p_dir_bin, $archive_name);
                if (!$l_error and -f $temp_archive_fullpath){
                    mylogln('移动文件到输出目录: ' . $temp_archive_fullpath . '=>' . $p_dir_out);
                    if (!move($temp_archive_fullpath, $p_dir_out)) {
                        $l_error = 1;
                        output_error("移动文件到输出目录失败! 压缩包：" . $temp_archive_fullpath) or die;
                    }
                } else {
                    $l_error = 1;
                    output_error('压缩文件失败！') or die;
                }
            } else {
                showMessageLn("已存在，无需打包。");
            }

            if ($p_include_ota) {
                showMessageLn("\n导出 OTA 文件"); 
                my @l_ota_files = get_ota_files($p_mtk_product, @l_ota_dirs);
                if (@l_ota_files > 0) {
                    my $l_ota_target_path = catfile($p_dir_out, 'OTA');
                    $l_ota_target_path = catfile($l_ota_target_path, $archive_base_name);
                    showMessageLn("导出到: " . $l_ota_target_path);
                    if (!-d $l_ota_target_path) {
                        $l_error = !mkpath($l_ota_target_path);
                        if ($l_error) {
                            output_error("创建 OTA 导出目录失败! $l_ota_target_path") or die;    # 创建目录失败
                        }    
                    }

                    if (-d $l_ota_target_path) {
                        system("sleep 0.01");     # 让此前的信息全部输出到命令行窗口
                        foreach $l_source_file (@l_ota_files) {
                            # ParryCopyFolderUtil::copyFile($l_source_file, $l_ota_target_path);
                            move($l_source_file, $l_ota_target_path) or die "移动文件失败: $!";
                        }

                        # 清除生成的 OTA 中间文件
                        my $ota_obj_dir = "$p_dir_bin/obj/PACKAGING/target_files_intermediates";
                        if (-d $ota_obj_dir) {
                            system("sleep 0.01");     # 让此前的信息全部输出到命令行窗口
                            # system("rm -rf \"" . $ota_obj_dir . "\"");
                            rmtree($ota_obj_dir);
                        }
                    }
                } else {
                    output_error('没发现 OTA 文件！') or die;
                }
            }
        }
    }
    
    showMessageLn("==================================================");
    
    if (!$l_error) {
        $retValue = catfile($p_dir_out, $archive_name);
    }

    return $retValue;
}

# 查找 Scatter 文件
sub find_scatter()
{
    my($p_dir) = @_;		# 获得传入参数
    
    my $scatterFileName = '';
    if (opendir (DH, $p_dir))
	{
		foreach $l_file(@currdir_files = readdir(DH))		# 列出所有目录或文件	
		{
            if ($l_file =~ /_Android_scatter.txt$/i) 
            {
                $scatterFileName = $l_file ;
                last;
            }
        }
    }
    else
	{
		output_error('目录不存在: ' . $p_dir) or die;
	}

    close(DH);
    
    return $scatterFileName;
}

# 分析 mtk 的 scatter 文件，获得需要的 Bin 文件
sub parsing_scatter
{
    my($p_fname_scatter) = @_;		# 获得传入参数
    my %filesHashMap = ();
    
    if (-f $p_fname_scatter)
    {
        my $filename_found = 0;
        my $is_download = 0;
        my $fname_bin = '';

        my $FILE_SCATTER    = $g_util->open_text_file( '<', $p_fname_scatter );
        my @scatter = <$FILE_SCATTER>;
        close($FILE_SCATTER);
    
        mylogln("分析 Scatter 文件: $p_fname_scatter");      # for test
        mylogln("文件行数: $#scatter");      # for test
        foreach my $line (@scatter)
        {
            # file_name: recovery.img
            if ($line =~ /^\s*file_name:\s*(.*)\s*/i)
            {
                $fname_bin = $1;
                if ($fname_bin !~/none/i)
                {
                    $filename_found = 1;
                }
                next;
            }
            
            # is_download: false
            if ($filename_found && $line =~ /^\s*is_download:\s*true\s*/i)
            {
                $is_download = 1;
            }
            
            if ($is_download)
            {
                $filesHashMap{$fname_bin} = '';     # 避免重复的文件名称
                
                $filename_found = 0;  
                $is_download = 0;
                $fname_bin = '';        
            }
        }        
    }
    
    my @file_list = keys(%filesHashMap);
    return @file_list;
}

# 添加数据库相关文件
sub add_db()
{
	my($p_dir, @p_file_prefix) = @_;		# 获得传入参数
	my $l_file;
	my $l_fullname;
	my @file_list = ();
	
	if (opendir (DH, $p_dir))
	{
        mylogln("扫描目录: $p_dir");      # for test
		foreach my $l_file (my @currdir_files = readdir DH)		# 列出所有目录或文件	
		{
			next if ($l_file eq "." or $l_file eq "..");
            next if ($l_file =~ /_enum$/i);
            
            $l_fullname = catfile($p_dir, $l_file);
            if (!-d $l_fullname)  # 如果不是目录
            {
                if (match_db_name($l_file, @p_file_prefix))
                {
                    mylogln("添加: ".$l_fullname);      # for test
                    push(@file_list, $l_fullname);
                }               
            }
            else
            {
                # 如果是目录，并且附合数据库的命名规则，则进入目录查找文件
                if (match_db_name($l_file, @p_file_prefix))
                {
                    my $sub_dir = $l_fullname;
                    if (opendir (SUB_DH, $sub_dir))
                    {
                        mylogln("扫描目录: $sub_dir");      # for test
                        foreach my $sub_file(my @subdir_files = readdir(SUB_DH))		# 列出所有目录或文件
                        {
                            next if ($sub_file eq "." or $sub_file eq "..");

                            if (match_db_name($sub_file, @p_file_prefix))
                            {
                                $l_fullname = catfile($sub_dir, $sub_file);
                                mylogln("添加: ".$l_fullname);
                                push(@file_list, $l_fullname);
                            }
                            
                        }
                    }
                    close(SUB_DH);
                }
            }
		}
		
		close(DH);						# 关闭目录
		
		if ($#file_list < 0)
		{
			showMessageLn("*** 目录中没找到数据库文件!" . $p_dir); 
			mylogln("*** 目录中没找到数据库文件！" . $p_dir);
		}
        
        mylogln("");      # for test        
	}
	else
	{
		mylogln("*** 目录不存在: $p_dir");
		showMessageLn("*** 目录不存在: $p_dir");
	}
    
    return @file_list;
}

# 搜索 OTA 文件
sub get_ota_files()
{
    my ($p_mtk_product, @l_ota_dirs) = @_;
    mylogln(Data::Dumper->Dump([\@l_ota_dirs], ['*' ."l_ota_dirs"]));

    my @l_ota_files = ();
    foreach my $tmp_dir (@l_ota_dirs) {
        if (opendir (DH, $tmp_dir)) {
            my @currdir_files = ();
            foreach $l_file (@tmp_array = readdir(DH)) {
                next if ($l_file eq '.');
                next if ($l_file eq '..');
                if ($l_file =~ /^$p_mtk_product-(target_files|ota)-.+\.zip$/i) {
                    push(@currdir_files, catfile($tmp_dir, $l_file));
                }
            }

            if (@currdir_files > 0) {
                # 按文件修改时间排序
                my @sortfiles = sort{
                    @sta1=stat($a);
                    @sta2=stat($b);
                    $sta2[9]<=>$sta1[9];    # 降序
                    # $sta1[9]<=>$sta2[9];    # 升序
                } @currdir_files;
                
                # 获取最新的 zip 文件
                push(@l_ota_files, $sortfiles[0]);
            } else {
                output_error('目录不存在 OTA 文件: ' . $tmp_dir);
            }
        }
        else {
            output_error('目录不存在: ' . $tmp_dir);
        }

        close(DH);
    }
    mylogln(Data::Dumper->Dump([\@l_ota_files], ['*' ."l_ota_files"]));
    return @l_ota_files;
}

sub match_db_name()
{
    my ($p_fname, @p_file_prefix) = @_;
    
    my $is_match = 0;
    foreach my $l_prefix (@p_file_prefix) 
    {
        if ($p_fname =~ /^$l_prefix.*/i) 
        {
            $is_match = 1;
            # mylogln($p_fname.",符合查找条件: $l_prefix");
            last;
        }
    }
    
    return $is_match;
}

sub compress
{
    my ($p_method_id, $p_archive_name, $p_work_dir, @p_arryay_file_list) = @_;
    my $file_list = join (' ', @p_arryay_file_list);
    
    my $cmd_base = '';       # 压缩软件
    my $options = '';

    # linux 内置的 zip
    if ($p_method_id eq 'linux-zip'){
        # 命令行：zip -j -1 a.zip a.img b.img c.img
        # j: 忽略路径, 1: 压缩级别为 1
        $cmd_base = 'zip';
        $options = '-j -1';            # 487m 23 秒
    }

    # 7zip
    # 命令行：7za a -t7z a.7z a.img b.img c.img
    # 7z 格式不同参数的测试数据
    # lzma2: 可以充分利用多核 CPU，速度极快，但可能一些很旧的软件无法解压
    # a0: lzma2压缩级别为 0 级，即不压缩，但实际上是有压缩的
    # mx=5: 7z 格式的压缩级别为 5，可选值为：0、1、5、7、9，0 为不压缩，值超大，压缩率越大
    # mmt: 启用多线程
    # $options = '-t7z -m0=lzma2 -mmt';             # 376m 29 秒
    # $options = '-t7z -m0=lzma2:a0 -mmt';          # 393m 34 秒
    # $options = '-t7z -m0=lzma2:a0 -mx=5 -mmt';    # 393m 41 秒
    # $options = '-t7z -m0=lzma2:a0 -mx=1 -mmt';    # 421m 7 秒
    # $options = '-t7z -m0=lzma2:a1 -mx=1 -mmt';    # 410m 13 秒
    if ($p_method_id =~ /^7zip/){
        $cmd_base = '7za a';
        if ($p_method_id eq '7zip-zip-x0'){
            $options = '-tzip -mx0';       # 896m 4 秒, -mx0 只存储
        }
        if ($p_method_id eq '7zip-zip-x1'){
            $options = '-tzip -mx1';       # 471m 28 秒, -mx1 最低压缩率
        }
        if ($p_method_id eq '7zip-7z-x1'){
            $options = '-t7z -m0=lzma2 -mx=1 -mmt';   # 421m 7 秒
        }
        if ($p_method_id eq '7zip-7z-x5'){
            $options = '-t7z -m0=lzma2 -mx=5 -mmt';   # 376m 31 秒
        }
    }
    
    my $cmd = sprintf("%s %s %s %s", $cmd_base, $options, $p_archive_name, $file_list);  # for linux

    chdir ($p_work_dir);
    unlink $p_archive_name if (-f $p_archive_name);       # 如果存在, 删除原压缩文件

    my $startTime = localtime() if ($performanceChk);

    mylogln('执行命令: ' . $cmd);
    my $result = system("$cmd") >> 8;
    showMessageLn('执行结果：' . $result) if ($debug);

    if ($performanceChk) {
        my $seconds = (localtime() - $startTime);
        my $hour = int($seconds / 60 / 60);
        my $minute = int($seconds / 60 % 60);
        my $second = $seconds % 60;
        showMessageLn('耗时：' . sprintf("%02d:%02d:%02d", $hour, $minute, $second));
    }

    chdir ($curdir);

    return $result eq 0;
}

# 根据压缩方法ID，生成压缩包名称
sub build_archive_name
{
    my ($p_method_id, $p_archive_name) = @_;

    my $archive_name = '';
    if ($p_method_id =~ /^.+-zip/){
        $archive_name = $p_archive_name;
        # $archive_name .= '-' . $p_method_id if ($debug);   # for test
        $archive_name = $archive_name . '.zip';
    }
    if ($p_method_id =~ /^7zip-7z/){
        $archive_name = $p_archive_name;
        # $archive_name .= '-' . $p_method_id if ($debug);   # for test
        $archive_name = $archive_name . '.7z';
    }

    return $archive_name;
}

sub check_compress_method
{
    my ($p_method_id) = @_;
    
    my $result = 0;
    foreach my $method_id (@compress_method) {
        if ($p_method_id eq $method_id) {
            $result = 1;
            last;
        }
    }

    return $result;
}

# 获得压缩包构成文件名的时间字符串
sub getTimeStrForArchiveName
{
    my ($time) = @_;

    my $arrayTime = localtime();
    if (defined($time)){
       $arrayTime = localtime($time);
    }
    my $startTime = localtime();
    return $arrayTime->strftime("%Y%m%d_%H%M"); 
}

sub mylogln
{
    my ($msg) = @_;
    
    $g_util->log("$msg\n") if ($debug);
}
sub mylog
{
    my ($msg) = @_;
    
    $g_util->log("$msg") if ($debug);
}

# 同时在显示器和文件输出
sub output_error
{
	my($p_string) = @_;		# 获得传入参数
	showMessageLn("*** ERROR: " . $p_string);
	# output_report($p_string);
    mylogln("*** ERROR: " . $p_string);
}

# 在文件输出消息
sub output_report
{
	my($p_string) = @_;		# 获得传入参数
	$g_util->output_result("$p_string\n");
}

####################################################################
# 获取文件主杆，含扩展名
sub just_fname_stem
{
	my($p_fullpath) = @_;		# 获得传入参数
	$p_fullpath =~ s#\\#/#g;	# 把 \ 替换为 /, 以兼容两类路径分隔符号
	return substr($p_fullpath, rindex($p_fullpath, "/")+1);  #  截取文件名，含扩展名
}
	
# 获取文件扩展名，不含点号
sub just_fname_ext
{
	my($p_fullpath) = @_;		# 获得传入参数
	my $fname = "";
	my $pos = -1;

	$p_fullpath =~ s#\\#/#g;	# 把 \ 替换为 /, 以兼容两类路径分隔符号
	$fname = substr($p_fullpath, rindex($p_fullpath, "/")+1);  #  截取文件名，含扩展名
	
	$pos = rindex($fname, "\.");
	return ($pos == -1 ? "" : substr($fname, $pos + 1));  #  截取扩展名
}

# 获取文件路径
sub just_fname_path
{
	my($p_fullpath) = @_;		# 获得传入参数
	$p_fullpath =~ s#\\#/#g;	# 把 \ 替换为 /, 以兼容两类路径分隔符号
	return substr($p_fullpath, 0, rindex($p_fullpath, "/"));  #  截取路径
}

return 1;	# 必须的返回命令
