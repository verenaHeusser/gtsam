/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file   GaussianBayesNet.cpp
 * @brief  Chordal Bayes Net, the result of eliminating a factor graph
 * @author Frank Dellaert
 */

#include <gtsam/linear/GaussianBayesNet.h>
#include <gtsam/linear/GaussianFactorGraph.h>
#include <gtsam/inference/FactorGraph-inst.h>
#include <gtsam/base/timing.h>

#include <boost/foreach.hpp>

using namespace std;
using namespace gtsam;

#define FOREACH_PAIR( KEY, VAL, COL) BOOST_FOREACH (boost::tie(KEY,VAL),COL) 
#define REVERSE_FOREACH_PAIR( KEY, VAL, COL) BOOST_REVERSE_FOREACH (boost::tie(KEY,VAL),COL)

namespace gtsam {

  // Instantiate base class
  template class FactorGraph<GaussianConditional>;

  /* ************************************************************************* */
  bool GaussianBayesNet::equals(const This& bn, double tol) const
  {
    return Base::equals(bn, tol);
  }

  /* ************************************************************************* */
  VectorValues GaussianBayesNet::optimize() const
  {
    VectorValues soln; // no missing variables -> just create an empty vector
    return optimize(soln);
  }

  /* ************************************************************************* */
  VectorValues GaussianBayesNet::optimize(
      const VectorValues& solutionForMissing) const {
    VectorValues soln(solutionForMissing); // possibly empty
    // (R*x)./sigmas = y by solving x=inv(R)*(y.*sigmas)
    /** solve each node in turn in topological sort order (parents first)*/
    BOOST_REVERSE_FOREACH(const sharedConditional& cg, *this) {
      // i^th part of R*x=y, x=inv(R)*y
      // (Rii*xi + R_i*x(i+1:))./si = yi <-> xi = inv(Rii)*(yi.*si - R_i*x(i+1:))
      soln.insert(cg->solve(soln));
    }
    return soln;
  }

  /* ************************************************************************* */
  VectorValues GaussianBayesNet::optimizeGradientSearch() const
  {
    gttic(GaussianBayesTree_optimizeGradientSearch);
    return GaussianFactorGraph(*this).optimizeGradientSearch();
  }

  /* ************************************************************************* */
  VectorValues GaussianBayesNet::gradient(const VectorValues& x0) const {
    return GaussianFactorGraph(*this).gradient(x0);
  }

  /* ************************************************************************* */
  VectorValues GaussianBayesNet::gradientAtZero() const {
    return GaussianFactorGraph(*this).gradientAtZero();
  }

  /* ************************************************************************* */
  double GaussianBayesNet::error(const VectorValues& x) const {
    return GaussianFactorGraph(*this).error(x);
  }

  /* ************************************************************************* */
  VectorValues GaussianBayesNet::backSubstitute(const VectorValues& rhs) const
  {
    VectorValues result;
    BOOST_REVERSE_FOREACH(const sharedConditional& cg, *this) {
      result.insert(cg->solveOtherRHS(result, rhs));
    }
    return result;
  }


  /* ************************************************************************* */
  // gy=inv(L)*gx by solving L*gy=gx.
  // gy=inv(R'*inv(Sigma))*gx
  // gz'*R'=gx', gy = gz.*sigmas
  VectorValues GaussianBayesNet::backSubstituteTranspose(const VectorValues& gx) const
  {
    // Initialize gy from gx
    // TODO: used to insert zeros if gx did not have an entry for a variable in bn
    VectorValues gy = gx;

    // we loop from first-eliminated to last-eliminated
    // i^th part of L*gy=gx is done block-column by block-column of L
    BOOST_FOREACH(const sharedConditional& cg, *this)
      cg->solveTransposeInPlace(gy);

    return gy;
  }

  ///* ************************************************************************* */
  //VectorValues GaussianBayesNet::optimizeGradientSearch() const
  //{
  //  gttic(Compute_Gradient);
  //  // Compute gradient (call gradientAtZero function, which is defined for various linear systems)
  //  VectorValues grad = gradientAtZero();
  //  double gradientSqNorm = grad.dot(grad);
  //  gttoc(Compute_Gradient);

  //  gttic(Compute_Rg);
  //  // Compute R * g
  //  Errors Rg = GaussianFactorGraph(*this) * grad;
  //  gttoc(Compute_Rg);

  //  gttic(Compute_minimizing_step_size);
  //  // Compute minimizing step size
  //  double step = -gradientSqNorm / dot(Rg, Rg);
  //  gttoc(Compute_minimizing_step_size);

  //  gttic(Compute_point);
  //  // Compute steepest descent point
  //  scal(step, grad);
  //  gttoc(Compute_point);

  //  return grad;
  //}

  /* ************************************************************************* */  
  pair<Matrix,Vector> GaussianBayesNet::matrix() const
  {
    return GaussianFactorGraph(*this).jacobian();
  }

  ///* ************************************************************************* */
  //VectorValues GaussianBayesNet::gradient(const VectorValues& x0) const
  //{
  //  return GaussianFactorGraph(*this).gradient(x0);
  //}

  ///* ************************************************************************* */
  //VectorValues GaussianBayesNet::gradientAtZero() const
  //{
  //  return GaussianFactorGraph(*this).gradientAtZero();
  //}

  /* ************************************************************************* */
  double GaussianBayesNet::determinant() const
  {
    return exp(logDeterminant());
  }

  /* ************************************************************************* */
  double GaussianBayesNet::logDeterminant() const
  {
    double logDet = 0.0;
    BOOST_FOREACH(const sharedConditional& cg, *this) {
      if(cg->get_model()) {
        Vector diag = cg->get_R().diagonal();
        cg->get_model()->whitenInPlace(diag);
        logDet += diag.unaryExpr(ptr_fun<double,double>(log)).sum();
      } else {
        logDet += cg->get_R().diagonal().unaryExpr(ptr_fun<double,double>(log)).sum();
      }
    }
    return logDet;
  }

  /* ************************************************************************* */

} // namespace gtsam
