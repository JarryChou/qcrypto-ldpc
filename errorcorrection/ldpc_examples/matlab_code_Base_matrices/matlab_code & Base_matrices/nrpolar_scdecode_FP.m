%Reliability sequence
Q=[0 1 2 4 8 16 32 3 5 64 9 6 17 10 18 128 12 33 65 20 256 34 24 36 7 129 66 512 11 40 68 130 ...
    19 13 48 14 72 257 21 132 35 258 26 513 80 37 25 22 136 260 264 38 514 96 67 41 144 28 69 42 ...
    516 49 74 272 160 520 288 528 192 544 70 44 131 81 50 73 15 320 133 52 23 134 384 76 137 82 56 27 ...
    97 39 259 84 138 145 261 29 43 98 515 88 140 30 146 71 262 265 161 576 45 100 640 51 148 46 75 266 273 517 104 162 ...
    53 193 152 77 164 768 268 274 518 54 83 57 521 112 135 78 289 194 85 276 522 58 168 139 99 86 60 280 89 290 529 524 ...
    196 141 101 147 176 142 530 321 31 200 90 545 292 322 532 263 149 102 105 304 296 163 92 47 267 385 546 324 208 386 150 153 ...
    165 106 55 328 536 577 548 113 154 79 269 108 578 224 166 519 552 195 270 641 523 275 580 291 59 169 560 114 277 156 87 197 ...
    116 170 61 531 525 642 281 278 526 177 293 388 91 584 769 198 172 120 201 336 62 282 143 103 178 294 93 644 202 592 323 392 ...
    297 770 107 180 151 209 284 648 94 204 298 400 608 352 325 533 155 210 305 547 300 109 184 534 537 115 167 225 326 306 772 157 ...
    656 329 110 117 212 171 776 330 226 549 538 387 308 216 416 271 279 158 337 550 672 118 332 579 540 389 173 121 553 199 784 179 ...
    228 338 312 704 390 174 554 581 393 283 122 448 353 561 203 63 340 394 527 582 556 181 295 285 232 124 205 182 643 562 286 585 ...
    299 354 211 401 185 396 344 586 645 593 535 240 206 95 327 564 800 402 356 307 301 417 213 568 832 588 186 646 404 227 896 594 ...
    418 302 649 771 360 539 111 331 214 309 188 449 217 408 609 596 551 650 229 159 420 310 541 773 610 657 333 119 600 339 218 368 ...
    652 230 391 313 450 542 334 233 555 774 175 123 658 612 341 777 220 314 424 395 673 583 355 287 183 234 125 557 660 616 342 316 ...
    241 778 563 345 452 397 403 207 674 558 785 432 357 187 236 664 624 587 780 705 126 242 565 398 346 456 358 405 303 569 244 595 ...
    189 566 676 361 706 589 215 786 647 348 419 406 464 680 801 362 590 409 570 788 597 572 219 311 708 598 601 651 421 792 802 611 ...
    602 410 231 688 653 248 369 190 364 654 659 335 480 315 221 370 613 422 425 451 614 543 235 412 343 372 775 317 222 426 453 237 ...
    559 833 804 712 834 661 808 779 617 604 433 720 816 836 347 897 243 662 454 318 675 618 898 781 376 428 665 736 567 840 625 238 ...
    359 457 399 787 591 678 434 677 349 245 458 666 620 363 127 191 782 407 436 626 571 465 681 246 707 350 599 668 790 460 249 682 ...
    573 411 803 789 709 365 440 628 689 374 423 466 793 250 371 481 574 413 603 366 468 655 900 805 615 684 710 429 794 252 373 605 ...
    848 690 713 632 482 806 427 904 414 223 663 692 835 619 472 455 796 809 714 721 837 716 864 810 606 912 722 696 377 435 817 319 ...
    621 812 484 430 838 667 488 239 378 459 622 627 437 380 818 461 496 669 679 724 841 629 351 467 438 737 251 462 442 441 469 247 ...
    683 842 738 899 670 783 849 820 728 928 791 367 901 630 685 844 633 711 253 691 824 902 686 740 850 375 444 470 483 415 485 905 ...
    795 473 634 744 852 960 865 693 797 906 715 807 474 636 694 254 717 575 913 798 811 379 697 431 607 489 866 723 486 908 718 813 ...
    476 856 839 725 698 914 752 868 819 814 439 929 490 623 671 739 916 463 843 381 497 930 821 726 961 872 492 631 729 700 443 741 ...
    845 920 382 822 851 730 498 880 742 445 471 635 932 687 903 825 500 846 745 826 732 446 962 936 475 853 867 637 907 487 695 746 ...
    828 753 854 857 504 799 255 964 909 719 477 915 638 748 944 869 491 699 754 858 478 968 383 910 815 976 870 917 727 493 873 701 ...
    931 756 860 499 731 823 922 874 918 502 933 743 760 881 494 702 921 501 876 847 992 447 733 827 934 882 937 963 747 505 855 924 ...
    734 829 965 938 884 506 749 945 966 755 859 940 830 911 871 639 888 479 946 750 969 508 861 757 970 919 875 862 758 948 977 923 ...
    972 761 877 952 495 703 935 978 883 762 503 925 878 735 993 885 939 994 980 926 764 941 967 886 831 947 507 889 984 751 942 996 ...
    971 890 509 949 973 1000 892 950 863 759 1008 510 979 953 763 974 954 879 981 982 927 995 765 956 887 985 997 986 943 891 998 766 ...
    511 988 1001 951 1002 893 975 894 1009 955 1004 1010 957 983 958 987 1012 999 1016 767 989 1003 990 1005 959 1011 1013 895 1006 1014 1017 1018 ...
    991 1020 1007 1015 1019 1021 1022 1023]+1;

N = 1024;
K = 512;
n = log2(N);
EbNodB = 3;
rmax = 4; %max received value
maxqr = 31; %max integer received value

Rate = K/N;
EbNo = 10^(EbNodB/10);
sigma = sqrt(1/(2*Rate*EbNo));

Q1 = Q(Q<=N); %reliability sequence for N

F = Q1(1:N-K); %Frozen positions: Q1(1:N-K)
%Message positions: Q1(N-K+1:end)

%Simulate
Nbiterrs = 0; Nblkerrs = 0; Nblocks = 20000;
parfor blk = 1:Nblocks
    msg = randi([0 1],1,K); %generate random K-bit message
    
    u = zeros(1,N);
    
    u(Q1(N-K+1:end)) = msg; %assign message bits
    
    m = 1; %number of bits combined
    for d = n-1:-1:0
        for i = 1:2*m:N
            a = u(i:i+m-1); %first part
            b = u(i+m:i+2*m-1); %second part
            u(i:i+2*m-1) = [mod(a+b,2) b]; %combining
        end
        m = m * 2;
    end
    cword = u;
    
    s = 1 - 2 * cword; %BPSK bit to symbol conversion
    r = s + sigma * randn(1,N); %AWGN channel I
    %quantization
    rq = floor(r/rmax*maxqr);
    rq(rq>maxqr) = maxqr;
    rq(rq<-(maxqr+1)) = -(maxqr+1);
    
    %SC decoder
    L = zeros(n+1,N); %beliefs
    ucap = zeros(n+1,N); %decisions
    ns = zeros(1,2*N-1); %node state vector
    
    satx = @(x) min(max(x,-(maxqr+1)),maxqr); %saturate FP value
    f = @(a,b) (1-2*(a<0)).*(1-2*(b<0)).*min(abs(a),abs(b)); %minsum
    g = @(a,b,c) satx(b+(1-2*c).*a); %g function
    
    L(1,:) = rq; %belief of root
    
    node = 0; depth = 0; %start at root
    done = 0; %decoder has finished or not
    while (done == 0) %traverse till all bits are decoded
        %leaf or not
        if depth == n
            if any(F==(node+1)) %is node frozen
                ucap(n+1,node+1) = 0;
            else
                if L(n+1,node+1) >= 0
                    ucap(n+1,node+1) = 0;
                else
                    ucap(n+1,node+1) = 1;
                end
            end
            if node == (N-1)
                done = 1;
            else
                node = floor(node/2); depth = depth - 1;
            end
        else
            %nonleaf
            npos = (2^depth-1) + node + 1; %position of node in node state vector
            if ns(npos) == 0 %step L and go to left child
                %disp('L')
                %disp([node depth])
                temp = 2^(n-depth);
                Ln = L(depth+1,temp*node+1:temp*(node+1)); %incoming beliefs
                a = Ln(1:temp/2); b = Ln(temp/2+1:end); %split beliefs into 2
                node = node *2; depth = depth + 1; %next node: left child
                temp = temp / 2; %incoming belief length for left child
                L(depth+1,temp*node+1:temp*(node+1)) = f(a,b); %minsum and storage
                ns(npos) = 1;
            else
                if ns(npos) == 1 %step R and go to right child
                    %disp('R')
                    %disp([node depth])
                    temp = 2^(n-depth);
                    Ln = L(depth+1,temp*node+1:temp*(node+1)); %incoming beliefs
                    a = Ln(1:temp/2); b = Ln(temp/2+1:end); %split beliefs into 2
                    lnode = 2*node; ldepth = depth + 1; %left child
                    ltemp = temp/2;
                    ucapn = ucap(ldepth+1,ltemp*lnode+1:ltemp*(lnode+1)); %incoming decisions from left child
                    node = node *2 + 1; depth = depth + 1; %next node: right child
                    temp = temp / 2; %incoming belief length for right child
                    L(depth+1,temp*node+1:temp*(node+1)) = g(a,b,ucapn); %g and storage
                    ns(npos) = 2;
                else %step U and go to parent
                    temp = 2^(n-depth);
                    lnode = 2*node; rnode = 2*node + 1; cdepth = depth + 1; %left and right child
                    ctemp = temp/2;
                    ucapl = ucap(cdepth+1,ctemp*lnode+1:ctemp*(lnode+1)); %incoming decisions from left child
                    ucapr = ucap(cdepth+1,ctemp*rnode+1:ctemp*(rnode+1)); %incoming decisions from right child
                    ucap(depth+1,temp*node+1:temp*(node+1)) = [mod(ucapl+ucapr,2) ucapr]; %combine
                    node = floor(node/2); depth = depth - 1;
                end
            end
        end
    end
    
    msg_cap = ucap(n+1,Q1(N-K+1:end));
    
    %Counting errors
    Nerrs = sum(msg ~= msg_cap);
    if Nerrs > 0
        Nbiterrs = Nbiterrs + Nerrs;
        Nblkerrs = Nblkerrs + 1;
    end
end

BER_sim = Nbiterrs/K/Nblocks;
FER_sim = Nblkerrs/Nblocks;

disp([EbNodB FER_sim BER_sim Nblkerrs Nbiterrs Nblocks])	


















