#!/usr/bin/perl

# 复制目录树到源码目录

BEGIN { push @INC, './mytools' } 
# 需要 File-Path 模块
use File::Find;
use File::Path;		# mkpath 函数
use File::Copy;		# Copy 函数
use Digest::file qw(digest_file_hex);    # 计算文件 MD5
use Cwd; 		        # getcwd()
use ParryUtil;			# 提供实用函数, 如: 输出结果文件, 输出 Log 
use File::Spec::Functions;

################
my $g_util = ParryUtil->new();         	 # 提供实用函数
my $curdir = getcwd();      # 当前目录
my $debug = 0;               # 调试模式

ClearScreen();       # 清屏

# my $baseDir = "./a";
my $baseDir = ".";
my $prject = "";
my $prjectDir = "";
my $prjectsBaseDir = '../projects/';
# my $prjectDir = "../projects/cta_bef_240x240";
# my $prjectDir = "../projects/cta_reai_240x240";
# my $prjectDir = "../projects/bef_240_vinchip_pa_v20_it7252_ft6336_old_camera_gc2385";
# my $prjectDir = "../projects/reai_240_vinchip_pa_v20_it7252_ft6336_new_camera_sp250a_ips";
my %files = ();		# key: 源文件路径,  value: 路径
# chdir($prjectDir);


if ($debug)
{
    my $FILE_LOG    = $g_util->open_text_file( '>', '~copyFiles.log');
    $g_util->set_log_handle($FILE_LOG) if ($debug);                        # 设置输出的 log 文件
}


if ($#ARGV >= 0)	# 最少有 1 个参数
{
	$prject = shift(@ARGV);			# 提取第一个参数, 并将其从 @ARGV 删除
}

if ($prject eq "")
{
	showMessageLn("需要指定项目名称!");
	listPrjs();
	die "\n";
}

$prjectDir = $prjectsBaseDir.$prject;
if (!-d $prjectDir)
{
	showMessageLn("项目不存在: ".$prjectDir);
	listPrjs();
	die "\n";
}

$codeFilesDir = catfile($prjectDir,'codeFiles');
if (!-d $codeFilesDir)
{
	showMessageLn("项目代码目录不存在: ".$codeFilesDir);
	die "\n";
}

mylogln("################################");
mylogln("基础目录：" . $curdir);
mylogln("项目目录：" . $prjectDir);
mylogln("项目代码目录：" . $codeFilesDir . "\n");

$files = ();
find(\&on_found, $codeFilesDir);

while ( ($l_from_file, $l_parent) = each(%files) ) 
{
    next if (-d $l_from_file);
    
    mylogln("================================");
    
    my $l_to_file = $l_from_file;
    $l_to_file =~ s/^\Q$codeFilesDir/$baseDir/;
    mylogln("来源：$l_from_file\n目标：$l_to_file");
    
    my $need_copy = 1;
    if (! -f $l_from_file)
    {
        $need_copy = 0;
        mylogln("$l_from_file 来源文件不存在，不复制！");
    }
    else
    {
        if (-f $l_to_file)
        {
            my $hash = get_file_md5($l_from_file);
            my $hashTarget = get_file_md5($l_to_file);
            if ($hash eq $hashTarget)
            {
                $need_copy = 0;
                mylogln("目标文件内容相同，不复制！");
				showMessageLn($l_from_file." 目标文件内容相同，不复制！");
            }
        }
    }

    if ($need_copy)
    {
        my $l_error = copy_file($l_from_file, $l_to_file);
        if (!$l_error)
        {
            mylogln("已复制！");
            showMessageLn($l_from_file." 已复制！");
        }
        else
        {
            output_error("复制文件错误！$!");
        }
    }
}
 

exit 1;


sub on_found{
    $files{$File::Find::name} = $File::Find::dir;
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
sub copy_file
{
    my($p_from, $p_to) = @_;		# 获得传入参数

    # 检查源文件是否存在
    my $l_error = ! -e $p_from;
    if($l_error)
    {
        mylogln("$p_from 来源文件不存在!");
    }
    else  
    {
        # 创建目标目录
        my $l_to_file = just_fname_path($p_to) . '/';  # 截取路径
        if (! -d $l_to_file)
        {
            $l_error = !mkpath($l_to_file);
            if ($l_error)
            {
                mylogln("创建目标目录失败 $l_to_file: $!");
            }
        }

        
        # 复制文件
        if (-d $l_to_file)
        {
            $l_error = !copy($p_from, $l_to_file);
            if (!$l_error)
            {
                mylogln("$p_from 已复制");
            }
            else
            {
                mylogln("$p_from 复制文件错误! $!");
            }
        }
    }
    return $l_error;
}

sub listPrjs
{
	showMessageLn("可用的项目:");
	if (opendir (DH, $prjectsBaseDir))
	{
		foreach $l_file(@currdir_files = readdir DH)		# 列出所有目录或文件	
		{
			next if ($l_file eq "." or $l_file eq "..");
            if (-d ($prjectsBaseDir.$l_file))
			{
				showMessageLn("     " . $l_file);
			}
		}
	}
	close(DH);						# 关闭目录
	showMessageLn("");
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
	showMessageLn($p_string);
	# output_report($p_string);
    mylogln($p_string);
}

# 在显示器输出消息
# sub output_message
# {
	# my($p_string) = @_;		# 获得传入参数
	# $g_util->output_message("$p_string\n");
# }

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