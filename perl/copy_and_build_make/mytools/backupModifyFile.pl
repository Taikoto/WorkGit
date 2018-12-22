#!/usr/bin/perl

# 用于提取指定的文件，并建立与源目录相同的目录结构
# 指定的文件列表，文件列表第一行必须是基础路径(已过时)
# PARRY 2010.12.08
# 自动查询 git 已修改的文件，并导出
# PARRY 9:55 2017/5/22


BEGIN { push @INC, './alps/mytools' } 
# 需要 File-Path 模块
use File::Path;		# mkpath 函数
use File::Copy;		# Copy 函数
use Cwd; 		# getcwd()
use ParryUtil;					# 提供实用函数, 如: 输出结果文件, 输出 Log 

################
my $g_util = ParryUtil->new();         	 # 提供实用函数
my $curdir = getcwd();      # 当前脚本目录
$curdir = '.';
my $target_dir = $curdir."/~backupModifyFile/";
my $source_dir = $curdir."/";
my $filelist  = "~git_status.txt";
my $report  = "~backupModifyFileReport.log";		# 报表文件

# system("clear");
ParryUtil::ClearScreen();       # 清屏

# 打开报表文件
open (my $FH_REPORT, ">", $report) or die "不能打开或创建文件: $report\n\n";
$g_util->set_result_handle($FH_REPORT);

my $queryModify = 0;
if ($#ARGV >= 0)
{
	$queryModify = shift(@ARGV);			# 提取第一个参数, 并将其从 @ARGV 删除
}

showMessageLn("===========================================");
showMessageLn("导出 git 已修改的文件");
showMessageLn("===========================================");
if ($queryModify)
{
	showMessageLn("正在执行命令 git status 查询已修改的文件...");
	system("git status > " . $filelist);
}
else 
{
	showMessageLn("输入参数 '1' 可以查询已修改的文件，并导出");
}

open (my $FH_FILE_LIST, "<", $filelist) or die "*** 不能打开文件: $filelist, 请输入参数 '1' 以查询已修改的文件\n\n";

my %files = ();		# key: 源文件路径,  value: 目标路径
my $l_str = "";
my @all_lines = <$FH_FILE_LIST>;

# $source_dir = subtrim(shift(@all_lines));
# $source_dir =~ s#\\#/#g;	# 把 \ 替换为 /, 以兼容两类路径分隔符号
# $source_dir =~ s#/$##;		# 删除结尾的目录分隔符( / )

foreach $line (@all_lines)
{

	$l_error = 0;
	$line = subtrim($line); 
	
	next if ($line eq "");		# 忽略空行
	
	my $source_file = "";
	
	# modified:
#modified:   mtk6735/android/alps/device/nb/nb6735m_65u_l1/system.prop	
#	if ($line = m/\s*modified:\s+(.+)\s*/)  {
	next if ($line !~ m/modified:\s+(.+)\s*/ && $line !~ m/new file:\s+(.+)\s*/ );
		# showMessageLn($1);
		$source_file = $source_dir.$1;
# showMessageLn($line);
	# output_report($source_file);
	
	
	if (!exists($files{$source_file})) 	# 避免重复项
	{
		$l_str = just_fname_path($source_file) . '/';  # 截取路径
		# $l_str =~ s#$source_dir#/#;
		$l_str =~ s#$source_dir#$target_dir#;
		# output_report("==>" . $l_str);
		# $l_str = $target_dir.$l_str;
		$files{$source_file} = $l_str;
	}
}
# output_both("\n");
	# while ( ($l_filepath, $l_path) = each(%files) ) 
	# {
		# output_both($l_filepath ."|". $l_path);
	# }
# exit 0;

my $l_files_cnt = keys(%files) ;
if ($l_files_cnt > 0)		# 如果非空
{
	output_both("----------------------------------------------------------------------");
	output_both("来源目录: $source_dir");
	output_both("目标目录: $target_dir");
	output_both("----------------------------------------------------------------------");
	
	my $l_error = 0;
	my $l_err_cnt = 0;
	my $l_copy_cnt = 0;
	while ( ($l_filepath, $l_path) = each(%files) ) 
	{
		# output_both("$l_filepath => $l_path");
		
		# 检查源文件是否存在
		$l_error = ! -e $l_filepath;
		if($l_error)
		{
			output_both("$l_filepath 来源文件不存在!");
		}
		
		# 创建目录
		if (!$l_error)
		{
			if (! -d $l_path)
			{
				$l_error = !mkpath($l_path);
				if ($l_error)
				{
					output_both("创建目录失败 $l_path: $!");
				}
			}
		}
			
		# 复制文件
		if (!$l_error)
		{
			$l_error = !copy($l_filepath, $l_path);
			if (!$l_error)
			{
				output_both("$l_filepath 已复制");
			}
			else
			{
				output_both("$l_filepath 复制文件错误! $!");
			}
		}
		
		# 统计错误数量
		$l_err_cnt++ if ($l_error);
	}

	$l_copy_cnt = $l_files_cnt - $l_err_cnt;
	output_both("----------------------------------------------------------------------");
	output_both("已复制: $l_copy_cnt");
	output_both("错误: $l_err_cnt");
	showMessageLn("报表: $report ");
	output_both("----------------------------------------------------------------------");
}
else
{
	output_both("================");
	output_both("无需要备份的文件");
	output_both("================");
	output_both("\n");
}

close FILE_SOURCE;

# 同时在显示器和文件输出
sub output_both
{
	my($p_string) = @_;		# 获得传入参数
	showMessageLn($p_string);
	output_report($p_string);
}

# 在显示器输出消息
# sub output_message
# {
	# my($p_string) = @_;		# 获得传入参数
	# print STDOUT $p_string;
	# $g_util->output_message("$p_string\n");
# }

# 在文件输出消息
sub output_report
{
	my($p_string) = @_;		# 获得传入参数
	# print FH_REPORT $p_string;
	$g_util->output_result("$p_string\n");
}

##############################################
# 实用函数
##############################################

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

# 清除两边空字符
sub subtrim
{
	my($p_string) = @_;		# 获得传入参数
	$p_string =~ s/^\s+//; 
	$p_string =~ s/\s+$//; 
	return $p_string;
}
