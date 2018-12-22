#!/usr/bin/perl

# 打包 bin 为 zip 文件, 名称例如: j7296_20140507.zip
# 新建项目时，需要指定项目名称 $prj_name
# 传入参数为“0”时，不打包数据库
# PARRY 19:51 2014-5-7
# PARRY 13:50 2014-5-16 增加打包数据库文件 
# PARRY 16:23 2017-3-18 增加 MT6735 数据库目录，纠正搜索文件算法错误

package ParryMTKAndroidUtil;
require Exporter;
@ISA = qw(Exporter);

# @EXPORT 可被外部程序访问的函数, 其它未定义的都是私有, 不可被外部访问
@EXPORT = qw(package_bin);
# @EXPORT_OK = qw($myvar1 $myvar2);	 # 可被外部程序访问的变量
 
use File::Path qw(mkpath rmtree);    # 创建文件夹树
# use File::Copy;                      # 复制文件

# use Data::Dumper;
use File::Spec::Functions;
use Cwd;

use utf8;

use ParryUtil;					# 提供实用函数, 如: 输出结果文件, 输出 Log 

# ParryUtil::ClearScreen();       # 清屏 

################
my $g_util = ParryUtil->new();         	 # 提供实用函数

my $debug = 0;      # 调试模式
my $FILE_LOG;
my $curdir = getcwd();      # 当前脚本目录

if ($debug)
{
    $FILE_LOG    = $g_util->open_text_file( '>', '~ParryMTKAndroidUtil.log');
    $g_util->set_log_handle($FILE_LOG) if ($debug);                        # 设置输出的 log 文件
}

sub package_bin
{
    my ($p_prj_name, $p_prj_name_mtk, $p_dir_bin_base, $p_dir_out, $p_fname_scatter, $p_build_mode, $p_include_database) = @_;
    
    my $author = ParryUtil->getSysUserName();   # 作者名称
    my $dir_bin = catfile( $p_dir_bin_base, $p_prj_name_mtk);
    my $l_error = 0;
    my @l_filelist = ();    # 将要被打包的文件
    my @l_not_exist = ();   # 缺少的文件

    # 数据库文件路径
    # MTK 描述
    # mediatek/source/cgen/APDB_$platform_SXX_ALPS_XX
    # mediatek/source/cgen/APDB_$platform_SXX_ALPS_XX.ENUM

    # MT6735
    # out/target/product/nb6735m_65u_l1/obj/CGEN/APDB_MT6735_S01_L1.MP3_W15.47

    # 实际路径
    # MT6572: 
    # mediatek/cgen/APDB_MT6572_S01_MAIN2.1_W10.24
    # mediatek/config/out/sanstar72_wet_lca/modem/BPLGUInfoCustomAppSrcP_MT6572_S00_MOLY_WR8_W1315_MD_WG_MP_V1_P3_1_2g_n

    # MT6582:
    # mediatek/custom/common/modem/keytak82_cwet_kk_hspa/BPLGUInfoCustomAppSrcP_MT6582_S00_MOLY_WR8_W1315_MD_WG_MP_V44_P3_1_wg_n

    # MT6735:
    # out/target/product/nb6735m_65u_l1/system/etc/mddb/BPLGUInfoCustomAppSrcP_MT6735_S00_MOLY_LR9_W1444_MD_LWTG_MP_V16_P11_1_lwg_n


    # 数据库目录及名称规则    
    my @l_database_dirs = (
    "out/target/product/$p_prj_name_mtk/system/etc/mddb",   # MT6735
    "out/target/product/$p_prj_name_mtk/obj/CGEN",          # MT6735
    # 'mediatek/cgen',
    # "mediatek/config/out/$p_prj_name_mtk/modem",
    );

    my @l_database_files = (
    'APDB_',
    'BPLGUInfoCustomAppSrcP_',
    );

    my $fname_scatter_fullpath = '';
    $fname_scatter_fullpath = catfile( $dir_bin, $p_fname_scatter );
    
    output_message("==================================================");
    output_message("打包 bin 文件");
    output_message("==================================================");

    mylog("当前文件夹: $curdir");
    mylog("Bin 文件夹: $dir_bin");
    mylog("Scatter 文件: $fname_scatter_fullpath"); 
    
    output_message("当前文件夹: $curdir");
    output_message("Bin 文件夹: $dir_bin");
    output_message("Scatter 文件: $fname_scatter_fullpath"); 
    output_message("------------------------------------------");
    
    if (!-e $fname_scatter_fullpath)
    {
        output_message("*** Scatter 文件不存在，无法打包! $fname_scatter_fullpath");
        $l_error = 1;
    }

    if (!$l_error && (! -d $p_dir_out))
    {
        $l_error = !mkpath($p_dir_out);
        if ($l_error)
        {
            output_message("*** 创建文件夹失败! $p_dir_out");                       # 创建目录失败
        }
    }
    
    if (!$l_error)
    {    
        mylog("\n添加 Scatter 文件..."); 
        mylog($fname_scatter_fullpath);
        push (@l_filelist, $fname_scatter_fullpath);
        
        mylog("\n添加 Bin 文件..."); 
        my @l_filelist_bin = parsing_scatter($fname_scatter_fullpath);
        foreach my $fname_temp (@l_filelist_bin)
        {
            $fname_temp = catfile( $dir_bin, $fname_temp );
            if (-e $fname_temp)
            {
				push(@l_filelist,$fname_temp);
                mylog("$fname_temp"); 
            }
            else
            {
               push(@l_not_exist,$fname_temp);
            }
        }

        # 数据库
        if ($p_include_database)
        {
            mylog("\n添加 数据库 文件..."); 
            for my $i (0..$#l_database_dirs)
            {
				push (@l_filelist, add_db($l_database_dirs[$i], @l_database_files));
            }
        }
  
        # 生成压缩文件名称
        my $c_date_time = get_date_by_string();
        my $archive =  sprintf("%s_%s_%s_%s\.zip", $p_prj_name, $p_build_mode, $c_date_time, $author);
        $archive = catfile( $p_dir_out, $archive);

        # 执行压缩
        output_message("执行压缩打包: $archive"); 
        output_message("------------------------------------------");
        unlink $archive if (-e $archive);       # 如果存在, 删除原压缩文件
        compress($archive, @l_filelist);
        output_message("------------------------------------------");

        # 列出缺少的文件
        if ($#l_not_exist >= 0)
        {
            output_message("*** 缺少以下文件:"); 
            foreach my $fname_temp (@l_not_exist)
            {
                output_message("$fname_temp"); 
            }        
        }        
        
        output_message("完毕!"); 
    }
    
    output_message("==================================================");
}

# 分析 mtk 的 scatter 文件
sub parsing_scatter
{
    my($p_fname_scatter) = @_;		# 获得传入参数
    my @file_list = ();
    
    if (-e $p_fname_scatter)
    {
        my $FILE_SCATTER    = $g_util->open_text_file( '<', $p_fname_scatter );
        my @scatter = <$FILE_SCATTER>;
        my $filename_found = 0;
        my $is_download = 0;
        my $fname_bin = '';
    
        mylog("分析 Scatter 文件: $p_fname_scatter");      # for test
        mylog("文件行数: $#scatter");      # for test
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
                push (@file_list, $fname_bin);
                
                $filename_found = 0;  
                $is_download = 0;
                $fname_bin = '';        
            }
        }        
    }
    
    return @file_list;
}

# 添加数据库相关文件
sub add_db()
{
	my($p_dir, @p_file_prefix) = @_;		# 获得传入参数
	my $l_file;
	my $l_fullname;
	my @file_list = ();
	my @currdir_files = ();
	
	mylog("扫描目录: $p_dir");      # for test
	
	if (opendir (DH, $p_dir))
	{
		foreach $l_file(@currdir_files = readdir DH)		# 列出所有目录或文件	
		{
			next if ($l_file eq "." or $l_file eq "..");
            next if ($l_file =~ /_enum$/i);
            
            $l_fullname = catfile($p_dir, $l_file);
            if (!-d $l_fullname)  # 如果不是目录
            {
                my $l_tempfile = $l_file;
                mylog2($l_file);
				for my $j (0..$#p_file_prefix)
				{
					my $l_prefix = $p_file_prefix[$j];
					# mylog("prefix: $l_prefix");      # for test
					if ($l_tempfile =~ /^$l_prefix.*/i)
					{
						mylog2(",符合查找条件: $l_prefix");
						# mylog("$l_tempfile,符合查找条件: $l_prefix");      # for test
						# mylog("Add: ".$l_fullname);      # for test
						push(@file_list, $l_fullname);
						last;
					}
				}
				mylog2("\n");      # for test
            }
            else
            {
                # mylog($l_file . ",这是目录");      # for test
            }
		}
		mylog2("\n");      # for test
		
		close(DH);						# 关闭目录
		
		if ($#file_list < 0)
		{
			output_message("*** 没找到数据库文件!"); 
			mylog("*** 没找到数据库文件！");
		}		
	}
	else
	{
		mylog("*** 目录不存在: $p_dir");
		output_message("*** 目录不存在: $p_dir");
	}
    
    return @file_list;
}

sub get_date_by_string
{
	my ($sec,$min,$hour,$mday,$month,$year,$wday,$yday,$isdst) = localtime();

    $month++; 
    $year+=1900;

    return sprintf("%4d%02d%02d_%02d%02d", $year, $month, $mday, $hour, $min);
}

sub compress
{
    my ($p_fname_zip, @p_arryay_file_list) = @_;
    my $cmd = '';
    my $file_list = join (' ', @p_arryay_file_list);
    my $options = '-j';
    
    #zip -j a.zip a.img b.img c.img
    $cmd = sprintf("zip %s %s %s", $options, $p_fname_zip, $file_list);  # for linux
    
    mylog($cmd);
    system($cmd);
}

sub mylog
{
    my ($msg) = @_;
    
    $g_util->log("$msg\n") if ($debug);
}
sub mylog2
{
    my ($msg) = @_;
    
    $g_util->log("$msg") if ($debug);
}

sub output_message
{
    my ($msg) = @_;
    $g_util->output_message("$msg\n");
}
return 1;	# 必须的返回命令
