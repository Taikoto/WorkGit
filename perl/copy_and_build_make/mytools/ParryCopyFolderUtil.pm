#!/usr/bin/perl

# 复制目录树到源码目录
# 如果文件相同，则不复制
# PARRY 15:04 2018/12/06 优化 copyFile 函数，第二参数可以是目录路径

package ParryCopyFolderUtil;
use Exporter;
@ISA = qw(Exporter);

# @EXPORT 可被外部程序访问的函数, 其它未定义的都是私有, 不可被外部访问
@EXPORT = qw(copyFolder copyFile);
# @EXPORT_OK = qw($myvar1 $myvar2);	 # 可被外部程序访问的变量
 

# 需要 File-Path 模块
use File::Find qw(find);
use File::Path qw(mkpath);		# mkpath 函数
use File::Copy qw(copy);		# copy 函数
use File::Compare qw(compare);  # compare 函数，比较文件
use File::Spec::Functions;
use Digest::file qw(digest_file_hex);    # 计算文件 MD5

use ParryUtil;			# 提供实用函数, 如: 输出结果文件, 输出 Log 

################
my $g_util = ParryUtil->new();         	 # 提供实用函数
my $debug = 0;               # 调试模式
my %g_files = ();		# key: 源文件路径,  value: 路径

if ($debug)
{
    my $FILE_LOG    = $g_util->open_text_file( '>', '~ParryCopyFolderUtil.log');
    $g_util->set_log_handle($FILE_LOG) if ($debug);                        # 设置输出的 log 文件
}

# 复制文件夹
# 返回值： 
#    0  成功
#   <0  失败
sub copyFolder
{
    my ($pSourceDir, $pTargetDir) = @_;  # 来源文件夹， 目标文件夹

    if ($#_ + 1 < 2)	# 最少有 2 个参数
    {
        $prject = shift(@_);			# 提取第一个参数, 并将其从 @ARGV 删除
        showMessageLn("参数太少!");
        return -1;
    }
    
    if (!-d $pSourceDir)
    {
        showMessageLn("来源目录不存在：: ".$pSourceDir);
        return -2;
    }

    mylogln("### copyFolder ###");
    mylogln("来源目录：" . $pSourceDir);
    mylogln("目标目录：" . $pTargetDir . "\n");

    find(\&on_found, $pSourceDir);        # 查找文件

    while ( ($l_from_file, $l_parent) = each(%g_files) ) 
    {
        next if (-d $l_from_file);
        
        # mylogln("================================");
        
        my $l_to_file = $l_from_file;
        $l_to_file =~ s/^\Q$pSourceDir/$pTargetDir/;
        # mylogln("来源：$l_from_file\n目标：$l_to_file");
        
        # if (! -f $l_from_file)
        # {
            # mylogln("!!! 来源文件不存在，不复制！");
        # }
        # else
        # {
            # my $need_copy = 1;
            # if (-f $l_to_file)
            # {
                # my $hash = get_file_md5($l_from_file);
                # my $hashTarget = get_file_md5($l_to_file);
                # if ($hash eq $hashTarget)
                # if (compare($l_from_file, $l_to_file) == 0)  # 结果为 0 是相同
                # {
                    # $need_copy = 0;
                    # mylogln("!!! 目标文件内容相同，不复制！");
                    # showMessageLn($l_from_file." 目标文件内容相同，不复制！");
                # }
            # }
            
            # if ($need_copy)
            # {
                # my $l_error = copyFile($l_from_file, $l_to_file);
                # if (!$l_error)
                if (copyFile($l_from_file, $l_to_file) == 0)
                {
                    mylogln($l_from_file."已复制！");
                    # showMessageLn($l_from_file." 已复制！");
                }
                # else
                # {
                    # output_error("复制文件错误！$!");
                # }
            # }
        # }

    }

    return 0;
}

sub on_found{
    $g_files{$File::Find::name} = $File::Find::dir;
}

sub get_file_md5
{
    my($p_file) = @_;		# 获得传入参数
    my $hash = "";
    
    # 检查源文件是否存在
    my $l_error = ! -e $p_file;
    if($l_error)
    {
        mylogln("get_file_md5, $p_file 来源文件不存在!");
    }
    else 
    {
		$hash = digest_file_hex($p_file, "MD5");
    }  
    mylogln("MD5:" . $hash .'|'. $p_file);
    
    return $hash;
}

# 复制文件，目录不存在则创建
sub copyFile
{
    my($p_from, $p_to) = @_;		# 获得传入参数

    my $l_from_file = $p_from;
    my $l_to_file = $p_to;

    # 检查源文件是否存在
    my $l_error = ! -e $l_from_file;
    if($l_error)
    {
        mylogln("!!! 来源文件不存在! $l_from_file");
        return -1;
    }
    else  
    {
        my $need_copy = 1;
        my $l_to_dir = '';
        if (-f $l_to_file)
        {
            # my $hash = get_file_md5($l_from_file);
            # my $hashTarget = get_file_md5($l_to_file);
            # if ($hash eq $hashTarget)
            if (compare($l_from_file, $l_to_file) == 0) {  # 结果为 0 是相同
                $need_copy = 0;
                mylogln("!!! 目标文件内容相同，不复制！" . $l_from_file . ' => ' . $l_to_file);
                # showMessageLn($l_from_file." 目标文件内容相同，不复制！");
                return 1;
            }
        }
        else
        {
            if (-d $l_to_file) {
                # 如果目标路径是目录
                $l_to_dir = $l_to_file;
                $l_to_file = catfile($l_to_dir, just_fname_stem($l_from_file));
                if (compare($l_from_file, $l_to_file) == 0) { # 结果为 0 是相同
                    $need_copy = 0;
                    mylogln("!!! 目标文件内容相同，不复制！" . $l_from_file . ' => ' . $l_to_file);
                    # showMessageLn($l_from_file." 目标文件内容相同，不复制！");
                    return 1;
                }
            } else {
                # 如果目标路径不是文件和目录，则截取路径并创建目录
                $l_to_dir = just_fname_path($l_to_file) . '/';  # 截取路径

                if (! -d $l_to_dir)
                {
                    $l_error = !mkpath($l_to_dir);
                    if ($l_error)
                    {
                        mylogln("创建目标目录失败 $l_to_dir: $!");
                    }
                }
            }
        }
        
        # 复制文件
        if ((!$l_error) and $need_copy and (-d $l_to_dir))
        {
            $l_error = !copy($p_from, $l_to_dir);
            if ($l_error)
            {
                mylogln("$p_from 复制文件错误! $!");
                return -2;
            }
        }
    }
    
    return 0;
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

# return 1;	# 必须的返回命令
return 1;
