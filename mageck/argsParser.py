"""MAGeCK argument parser
Copyright (c) 2014 Wei Li, Han Xu, Xiaole Liu lab 
This code is free software; you can redistribute it and/or modify it
under the terms of the BSD License (see the file COPYING included with
the distribution).
@status:  experimental
@version: $Revision$
@author:  Wei Li 
@contact: li.david.wei AT gmail.com
"""


from __future__ import print_function
import sys
import argparse
import logging

def crisprseq_parseargs():
  """Parsing mageck arguments.
  """
  parser=argparse.ArgumentParser(description='mageck: performs sgRNA, gene and pathway analysis on CRISPR-Cas9 screening data.');
  # definition of sub commands
  subparser=parser.add_subparsers(help='commands to run mageck',dest='subcmd');
  
  parser.add_argument('-v', '--version',action='version',version='%(prog)s 0.5.0');
  
  # run: execute the whole program
  subp_run=subparser.add_parser('run',help='Run the whole program from fastq files.');
  subp_run.add_argument('-l','--list-seq',help='A file containing the list of sgRNA names, their sequences and associated genes. Support file format: csv and txt.');
  subp_run.add_argument('-n','--output-prefix',default='sample1',help='The prefix of the output file(s). Default sample1.');
  subp_run.add_argument('--sample-label',default='',help='Sample labels, separated by comma. Must be equal to the number of samples provided (in --fastq option). Default "sample1,sample2,...".');
  subp_run.add_argument('--trim-5',type=int,default=0,help='Length of trimming the 5\' of the reads. Default 0');
  subp_run.add_argument('--sgrna-len',type=int,default=20,help='Length of the sgRNA. Default 20');
  subp_run.add_argument('--count-n',action='store_true',help='Count sgRNAs with Ns. By default, sgRNAs containing N will be discarded.');
  #
  subp_run.add_argument('--gene-test-fdr-threshold',type=float,default=0.25,help='FDR threshold for gene test, default 0.25.');
  subp_run.add_argument('-t','--treatment-id',required=True,action='append',help='Sample label or sample index (integer, 0 as the first sample)  as treatment experiments, separated by comma (,). If sample label is provided, the labels must match the labels in the first line of the count table; for example, "HL60.final,KBM7.final". For sample index, "0,2" means the 1st and 3rd samples are treatment experiments.');
  subp_run.add_argument('-c','--control-id',action='append',help='Sample label or sample index in the count table as control experiments, separated by comma (,). Default is all the samples not specified in treatment experiments.');
  subp_run.add_argument('--adjust-method',choices=['fdr','holm'],default='fdr',help='Method for p-value adjustment, including false discovery rate (fdr) or holm\'s method (holm). Default fdr.');
  subp_run.add_argument('--variance-from-all-samples',action='store_true',help='Estimate the variance from all samples, instead of from only control samples. Use this option only if you believe there are relatively few essential sgRNAs or genes between control and treatment samples.');
  subp_run.add_argument('--keep-tmp',action='store_true',help='Keep intermediate files.');
  subp_run.add_argument('--fastq',nargs='+',help='Sample fastq files, separated by space; use comma (,) to indicate technical replicates of the same sample. For example, "--fastq sample1_replicate1.fastq,sample1_replicate2.fastq sample2_replicate1.fastq,sample2_replicate2.fastq" indicates two samples with 2 technical replicates for each sample.');
  subp_run.add_argument('--pdf-report',action='store_true',help='Generate pdf report of the analysis.');
  
  # countonly: only collect counts
  subp_count=subparser.add_parser('count',help='Collecting read counts from fastq files.');
  subp_count.add_argument('-l','--list-seq',required=True,help='A file containing the list of sgRNA names, their sequences and associated genes. Support file format: csv and txt.');
  subp_count.add_argument('--sample-label',default='',help='Sample labels, separated by comma (,). Must be equal to the number of samples provided (in --fastq option). Default "sample1,sample2,...".');
  subp_count.add_argument('-n','--output-prefix',default='sample1',help='The prefix of the output file(s). Default sample1.');
  subp_count.add_argument('--trim-5',type=int,default=0,help='Length of trimming the 5\' of the reads. Default 0');
  subp_count.add_argument('--sgrna-len',type=int,default=20,help='Length of the sgRNA. Default 20');
  subp_count.add_argument('--count-n',action='store_true',help='Count sgRNAs with Ns. By default, sgRNAs containing N will be discarded.');
  subp_count.add_argument('--fastq',nargs='+',help='Sample fastq files, separated by space; use comma (,) to indicate technical replicates of the same sample. For example, "--fastq sample1_replicate1.fastq,sample1_replicate2.fastq sample2_replicate1.fastq,sample2_replicate2.fastq" indicates two samples with 2 technical replicates for each sample.');
  subp_count.add_argument('--pdf-report',action='store_true',help='Generate pdf report of the fastq files.');
  subp_count.add_argument('--keep-tmp',action='store_true',help='Keep intermediate files.');
  
  # stat test: only do the statistical test
  subn_stattest=subparser.add_parser('test',help='Perform statistical test from a given count table (generated by count command).');
  subn_stattest.add_argument('-k','--count-table',required=True,help='Provide a tab-separated count table instead of sam files. Each line in the table should include sgRNA name (1st column) and read counts in each sample.');
  # this parameter is depreciated
  # subn_stattest.add_argument('--gene-test',help='Perform rank association analysis. A tab-separated, sgRNA to gene mapping file is required. Each line in the file should include two columns, the sgRNA name and the gene name.');
  subn_stattest.add_argument('-t','--treatment-id',required=True,action='append',help='Sample label or sample index (0 as the first sample) in the count table as treatment experiments, separated by comma (,). If sample label is provided, the labels must match the labels in the first line of the count table; for example, "HL60.final,KBM7.final". For sample index, "0,2" means the 1st and 3rd samples are treatment experiments.');
  subn_stattest.add_argument('-c','--control-id',action='append',help='Sample label or sample index in the count table as control experiments, separated by comma (,). Default is all the samples not specified in treatment experiments.');
  subn_stattest.add_argument('-n','--output-prefix',default='sample1',help='The prefix of the output file(s). Default sample1.');
  #parser.add_argument('--count-control-index',help='If -k/--counts option is given, this parameter defines the control experiments in the table.'); 
  subn_stattest.add_argument('--norm-method',choices=['none','median','total'],default='median',help='Method for normalization, default median.');
  subn_stattest.add_argument('--normcounts-to-file',action='store_true',help='Write normalized read counts to file ([output-prefix].normalized.txt).');
  subn_stattest.add_argument('--gene-test-fdr-threshold',type=float,default=0.25,help='FDR threshold for gene test, default 0.25.');
  subn_stattest.add_argument('--adjust-method',choices=['fdr','holm'],default='fdr',help='Method for p-value adjustment, including false discovery rate (fdr) or holm\'s method (holm). Default fdr.');
  subn_stattest.add_argument('--variance-from-all-samples',action='store_true',help='Estimate the variance from all samples, instead of from only control samples. Use this option only if you believe there are relatively few essential sgRNAs or genes between control and treatment samples.');
  subn_stattest.add_argument('--sort-criteria',choices=['neg','pos'],default='neg',help='Sorting criteria, either by negative selection (neg) or positive selection (pos). Default negative selection.');
  subn_stattest.add_argument('--keep-tmp',action='store_true',help='Keep intermediate files.');
  subn_stattest.add_argument('--control-sgrna',help='A list of control sgRNAs for generating null distribution.');
  subn_stattest.add_argument('--remove-zero',choices=['none','control','treatment','both'],default='none',help='Whether to remove zero-count sgRNAs in control and/or treatment experiments. Default: none (do not remove those zero-count sgRNAs).');
  subn_stattest.add_argument('--pdf-report',action='store_true',help='Generate pdf report of the analysis.');
  
  # pathway test
  subw_pathway=subparser.add_parser('pathway',help='Perform significant pathway analysis from gene rankings generated by the test command.');
  subw_pathway.add_argument('--gene-ranking',required=True,help='The gene summary file (containing both positive and negative selection tests) generated by the gene test step. Pathway enrichment will be performed in both directions.');
  subw_pathway.add_argument('--single-ranking',action='store_true',help='The provided file is a (single) gene ranking file, either positive or negative selection. Only one enrichment comparison will be performed.');
  # subw_pathway.add_argument('--gene-ranking-2',help='An optional gene ranking file of opposite direction, generated by the gene test step.');
  subw_pathway.add_argument('--gmt-file',required=True,help='The pathway file in GMT format.');
  subw_pathway.add_argument('-n','--output-prefix',default='sample1',help='The prefix of the output file(s). Default sample1.');
  subw_pathway.add_argument('--sort-criteria',choices=['neg','pos'],default='neg',help='Sorting criteria, either by negative selection (neg) or positive selection (pos). Default negative selection.');
  subw_pathway.add_argument('--keep-tmp',action='store_true',help='Keep intermediate files.');
  subw_pathway.add_argument('--ranking-column',default='2',help='Column number or label in gene summary file for gene ranking; can be either an integer of column number, or a string of column label. Default "2" (the 3rd column).');
  subw_pathway.add_argument('--ranking-column-2',default='7',help='Column number or label in gene summary file for gene ranking; can be either an integer of column number, or a string of column label. This option is used to determine the column for positive selections and is disabled if --single-ranking is specified. Default "5" (the 6th column).');
  
  # plot
  subp_plot=subparser.add_parser('plot',help='Generating graphics for selected genes.');
  subp_plot.add_argument('-k','--count-table',required=True,help='Provide a tab-separated count table instead of sam files. Each line in the table should include sgRNA name (1st column) and read counts in each sample.');
  subp_plot.add_argument('-g','--gene-summary',required=True,help='The gene summary file generated by the test command.');
  subp_plot.add_argument('--genes',help='A list of genes to be plotted, separated by comma. Default: none.');
  subp_plot.add_argument('-s','--samples',help='A list of samples to be plotted, separated by comma. Default: using all samples in the count table.');
  subp_plot.add_argument('-n','--output-prefix',default='sample1',help='The prefix of the output file(s). Default sample1.');
  subp_plot.add_argument('--norm-method',choices=['none','median','total'],default='median',help='Method for normalization, default median.');
  subp_plot.add_argument('--keep-tmp',action='store_true',help='Keep intermediate files.');
  
  args=parser.parse_args();
  
  # logging status
  if args.subcmd=='pathway':
    logmode="a";
  else:
    logmode="w";
  
  logging.basicConfig(level=10,
    format='%(levelname)-5s @ %(asctime)s: %(message)s ',
    datefmt='%a, %d %b %Y %H:%M:%S',
    # stream=sys.stderr,
    filename=args.output_prefix+'.log',
    filemode=logmode
  );
  console = logging.StreamHandler()
  console.setLevel(logging.INFO)
  # set a format which is simpler for console use
  formatter = logging.Formatter('%(levelname)-5s @ %(asctime)s: %(message)s ','%a, %d %b %Y %H:%M:%S')
  #formatter.formatTime('%a, %d %b %Y %H:%M:%S');
  # tell the handler to use this format
  console.setFormatter(formatter)
  # add the handler to the root logger
  logging.getLogger('').addHandler(console)
  
  # add paramters
  logging.info('Parameters: '+' '.join(sys.argv));
  
  return args;


