
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/core/core_c.h>
#include <vector>

#include <stdio.h>

using namespace cv;

// The enumeration of the different types of Norms comes from OpenCV 2.4.x
// Backported here for convience.
//enum { NORM_INF=1, NORM_L1=2, NORM_L2=4, NORM_L2SQR=5, NORM_HAMMING=6, NORM_HAMMING2=7, NORM_TYPE_MASK=7, NORM_RELATIVE=8, NORM_MINMAX=32 };
enum { NORM_L2SQR = 5, NORM_HAMMING=6 };

struct CvDMatch_t {
  int queryIdx;
  int trainIdx;
  int imgIdx;

  float distance;
  float ratio;
  unsigned int rank;
};

struct CvMatcherParams_t {
  int normType;
  int knn;
  bool calculateRatios;
  float minRatio;
  float minRadius;
  bool crossCheck;
};


enum { CONVERT_ALL = 0, TAKE_JUST_FIRST = 1 };

extern "C" {

  static void writeDmatchToSeqWriter( CvSeqWriter &writer, const DMatch &dmatch, unsigned int rank = 0, float ratio = 0.0 )
  {
    CvDMatch_t dm;
    dm.queryIdx = dmatch.queryIdx;
    dm.trainIdx = dmatch.trainIdx;
    dm.imgIdx   = dmatch.imgIdx;
    dm.distance = dmatch.distance;
    dm.ratio    = ratio;
    dm.rank     = rank;
    CV_WRITE_SEQ_ELEM( dm, writer );

  }

  CvSeq *DMatchToCvSeq( const vector< vector<DMatch> > &matches, CvMemStorage *storage, int doTakeFirst, bool calculateRatios CV_DEFAULT(false) )
  {
    // TODO:  For now, Knn will return a flattened set of matches, 
    // May want to change this behavior in the future
    CvSeq *seq = cvCreateSeq( 0, sizeof( CvSeq ), sizeof( CvDMatch_t ), storage );
    float ratio;

    CvSeqWriter writer;
    cvStartAppendToSeq( seq, &writer );
    for( vector< vector<DMatch> >::const_iterator itr = matches.begin(); itr != matches.end(); itr++ ) {

      if( doTakeFirst == TAKE_JUST_FIRST ) {
        if( !(*itr).empty() ) {

          if( !calculateRatios  || (*itr).size() == 1 )
            ratio = NAN;
          else
            ratio = (*itr)[1].distance/(*itr)[0].distance;

          writeDmatchToSeqWriter( writer, (*itr)[0], 0, ratio );
        }
      } else {

        unsigned int rank = 0;

        for( vector<DMatch>::const_iterator itr2 = (*itr).begin();  itr2 != (*itr).end(); itr2++ ) {
          if( calculateRatios ) {
            vector<DMatch>::const_iterator next = itr2 + 1;

            if( next != (*itr).end() ) {
              ratio = (*next).distance / (*itr2).distance;
              writeDmatchToSeqWriter( writer, (*itr2), rank, ratio );
            } else {
              writeDmatchToSeqWriter( writer, (*itr2), rank, NAN );
            }

          } else {
            writeDmatchToSeqWriter( writer, (*itr2), rank );
          }

          ++rank;
        }
      }
    }
    cvEndWriteSeq( &writer );

    //printf("After conversion, vector size = %d, CvSeq size = %d\n", 
    //    matches.size(), seq->total );

    //assert( matches.size() == (unsigned int)seq->total );

    return seq;
  }

  CvSeq *DMatchToCvSeqRatioTest( const vector< vector<DMatch> > &matches, CvMemStorage *storage, float minRatio )
  {

    // TODO:  For now, Knn will return a flattened set of matches,
    // may do this differently in the future
    CvSeq *seq = cvCreateSeq( 0, sizeof( CvSeq ), sizeof( CvDMatch_t ), storage );
    CvSeqWriter writer;
    cvStartAppendToSeq( seq, &writer );
    float ratio;

    for( vector< vector<DMatch> >::const_iterator itr = matches.begin(); itr != matches.end(); itr++ ) {

      switch( (*itr).size() ) {
        case 1:
          writeDmatchToSeqWriter( writer, (*itr)[0] );
          break;
        default:
          // Assumes the matches are sorted in increasing order of distance
          if( (*itr).size() >= 2 ) {
            ratio = (*itr)[1].distance/(*itr)[0].distance;
            //printf("Comparing distaces %f and %f (%f)", (*itr)[0].distance, (*itr)[1].distance, (*itr)[1].distance/(*itr)[0].distance );
            if( ratio > minRatio )  {
              //printf(" accept\n");
              writeDmatchToSeqWriter( writer, (*itr)[0], 0, ratio );
            } else {
              //printf(" reject\n");
            }
          }
          break;
      }
    }
    cvEndWriteSeq( &writer );

    //printf("After conversion, vector size = %d, CvSeq size = %d\n", 
    //    matches.size(), seq->total );

    return seq; 
  }



  //##### Brute Force Matcher #######
  void bruteForceMatcherKnnActual( CvMat *query, CvMat *train, vector< vector<DMatch> > &matches, int normType, int knn, bool crossCheck CV_DEFAULT(false) ) 
  {
    BFMatcher matcher( normType, crossCheck );
    matcher.knnMatch( query, train, matches, knn );
  }

  CvSeq *bruteForceMatcher( CvMat *query, CvMat *train, 
                               CvMemStorage *storage, int normType, 
                               bool crossCheck CV_DEFAULT(false) ) 
  {
    vector< vector<DMatch> > matches;
    bruteForceMatcherKnnActual( query, train, matches, normType, 1, crossCheck );
    return DMatchToCvSeq( matches, storage, TAKE_JUST_FIRST );
  }

  CvSeq *bruteForceMatcherKnn( CvMat *query, CvMat *train, 
                               CvMemStorage *storage, int normType, 
                               int knn, bool crossCheck CV_DEFAULT(false) ) 
  {
    vector< vector<DMatch> > matches;
    bruteForceMatcherKnnActual( query, train, matches, normType, knn, crossCheck );
    return DMatchToCvSeq( matches, storage, CONVERT_ALL );
  }

  CvSeq *bruteForceMatcherRatioTest( CvMat *query, CvMat *train, 
                                     CvMemStorage *storage, int normType, 
                                     float minRatio, bool crossCheck CV_DEFAULT(false) ) 
  {
    vector< vector<DMatch> > matches;
    bruteForceMatcherKnnActual( query, train, matches, normType, 2, crossCheck );
    return DMatchToCvSeqRatioTest( matches, storage, minRatio );
  }

  CvSeq *bruteForceMatcherRadius( CvMat *query, CvMat *train, 
                                  CvMemStorage *storage, int normType, 
                                  float maxDistance, bool crossCheck CV_DEFAULT(false) ) 
  {
    vector< vector<DMatch> > matches;

    BFMatcher matcher( normType, crossCheck );
    matcher.radiusMatch( query, train, matches, maxDistance );
    
    return DMatchToCvSeq( matches, storage, CONVERT_ALL );
  }


  // This is the "universal" function parameterized through params.
  CvSeq *bruteForceMatcherParams( CvMat *query, 
                                  CvMat *train, 
                                  CvMemStorage *storage,
                                  CvMatcherParams_t *params )
  {
    int knn = params->knn;
    if( params->calculateRatios && params->knn == 1 ) knn++;

    vector< vector<DMatch> > matches;
    BFMatcher matcher( params->normType, params->crossCheck );

    if (params->minRadius > 0.0 ) 
      matcher.radiusMatch( query, train, matches, params->minRadius ); 
    else {
      matcher.knnMatch( query, train, matches, knn );
    }

    if( params->minRatio > 0.0 ) {
      return DMatchToCvSeqRatioTest( matches, storage, params->minRatio );
    }

    return DMatchToCvSeq( matches, storage, 
        (params->knn == 1) ? TAKE_JUST_FIRST : CONVERT_ALL,
        params->calculateRatios );
  }

  //##### FLANN Matcher #######
  // TODO:  Expose flann parameters through API
  void flannMatcherKnnActual( CvMat *query, CvMat *train, vector< vector<DMatch> > &matches, int knn )
  {
    Mat _train( train );
    Mat _query( query );

    FlannBasedMatcher matcher;
    matcher.knnMatch( query, train, matches, knn );
  }

  CvSeq *flannBasedMatcher( CvMat *query, CvMat *train, CvMemStorage *storage )
  {
    vector< vector<DMatch> > matches;
    flannMatcherKnnActual( query, train, matches,  1 );
    return DMatchToCvSeq( matches, storage, TAKE_JUST_FIRST );
  }
  CvSeq *flannBasedMatcherKnn( CvMat *query, CvMat *train, CvMemStorage *storage, int knn )
  {
    vector< vector<DMatch> > matches;
    flannMatcherKnnActual( query, train, matches, knn );
    return DMatchToCvSeq( matches, storage, CONVERT_ALL );
  }

  CvSeq *flannBasedMatcherRatioTest( CvMat *query, CvMat *train, CvMemStorage *storage, float minRatio )
  {
    vector< vector<DMatch> > matches;
    flannMatcherKnnActual( query, train, matches, 2 );
   return DMatchToCvSeqRatioTest( matches, storage, minRatio );
  }

  CvSeq *flannBasedMatcherRadius( CvMat *query, CvMat *train, CvMemStorage *storage, float maxDistance )
  {
    Mat _train( train );
    Mat _query( query );
    vector< vector<DMatch> > matches;

    FlannBasedMatcher matcher;
    matcher.radiusMatch( query, train, matches, maxDistance );
    return DMatchToCvSeq( matches, storage, CONVERT_ALL );
  }



}
